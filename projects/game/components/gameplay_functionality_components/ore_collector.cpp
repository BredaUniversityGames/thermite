#include "ore_collector.hpp"

#include "projects/game/components/development_tools/collision_trigger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/wallet.hpp"
void game::OreCollector::start() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().connect<&OreCollector::on_collision_trigger>(this);
    tmt::engine.ecs.get_dispatcher().sink<EndRun>().connect<&OreCollector::on_end_run>(this);
    if (wallet_entity == entt::null) {
        wallet_entity = entity;
    }
}
void game::OreCollector::update(const tmt::FrameData& time) {}

void game::OreCollector::end() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().disconnect<&OreCollector::on_collision_trigger>(this);
    tmt::engine.ecs.get_dispatcher().sink<EndRun>().disconnect<&OreCollector::on_end_run>(this);

    transfer_money();
}

void game::OreCollector::on_collision_trigger(const TriggerCollisionEvent& trigger) {
    // TODO do something smarter maybe, check type of ore etc.
    if (trigger.trigger != entity) {
        return;
    }
    if (!tmt::engine.ecs.valid(trigger.other_object)) {
        return;
    }

    auto* vr = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(trigger.other_object);
    if (vr == nullptr) {
        tmt::Log::error("Entity has no voxel renderer and is collected!", trigger.other_object);
        tmt::engine.ecs.destroy_entity(trigger.other_object);

        return;
    }

    auto* voxel_tree = vr->resource.resource.get();
    if (voxel_tree == nullptr) {
        tmt::Log::error("Entity has no voxel body and is collected!", trigger.other_object);
        tmt::engine.ecs.destroy_entity(trigger.other_object);

        return;
    }
    auto& blas = voxel_tree->blas;
    // deframent, danger it might be very slow
    voxel_tree->blas->defrag();
    std::array<uint64_t, magic_enum::enum_count<tmt::Material::Type>()> type_counts {};
    type_counts.fill(0);

    for (uint32_t i = 0; i < blas->node_count; ++i) {
        // search for actual voxels only
        if (!blas->nodes[i].is_leaf()) continue;

        // get the actual voxel material ptr
        const uint32_t ptr = blas->nodes[i].abs_ptr();
        // just how many materials there are in that memory
        const uint32_t count = __popcnt64(blas->nodes[i].child_mask);

        for (uint32_t j = 0; j < count; ++j) {
            auto type = blas->palette.entries[blas->materials[ptr + j]].type;
            type_counts[static_cast<uint8_t>(type)]++;
        }
    }
    for (auto type : magic_enum::enum_values<tmt::Material::Type>()) {
        auto count = type_counts[static_cast<uint8_t>(type)];
        tmt::Log::info("Ore count is {}, for {}", count, magic_enum::enum_name(type));
    }
    tmt::engine.ecs.destroy_entity(trigger.other_object);

    // update the wallet
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(wallet_entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component found");

        return;
    }
    auto ore_view = tmt::engine.ecs.view<OreProperties>();
    if (ore_view.begin() == ore_view.end()) {
        tmt::Log::error("No OreProperties found");
        return;
    }

    auto& ore_properties = ore_view.front().get<OreProperties>();
    for (auto& [mat_type, ore] : ore_properties.ores) {
        auto count = type_counts[static_cast<uint8_t>(mat_type)];
        if (count == 0) continue;

        wallet->currencies.resource_counts[ore.ore_resource] += count * ore.resource_per_voxel;
    }
}
void game::OreCollector::on_end_run(const EndRun& end_run) {
    penalty = end_run.player_dead;
    if (end_run.player_dead == false) {
        penalty = false;
    }
}
void game::OreCollector::transfer_money() {
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(wallet_entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component found");

        return;
    }
    if (penalty) {
        tmt::Log::info("Penalty of {:.2f}, applied when saving current run resources", penalty_percentage);
        for (auto& pair : wallet->currencies.resource_counts) {
            pair.second = static_cast<uint64_t>(static_cast<float>(pair.second) * penalty_percentage);
        }
        wallet->currencies.dollars = static_cast<uint64_t>(static_cast<float>(wallet->currencies.dollars) * penalty_percentage);
    }

    tmt::Log::info("Dollars: {}", wallet->currencies.dollars);
    for (auto& pair : wallet->currencies.resource_counts) {
        tmt::Log::info("Resource {}, count {}", magic_enum::enum_name(pair.first), pair.second);
    }
    auto& resources { tmt::engine.player_data.get<Currencies>("Run resources") };
    resources = wallet->currencies;
    // reset state
    wallet->currencies = Currencies {};
}
