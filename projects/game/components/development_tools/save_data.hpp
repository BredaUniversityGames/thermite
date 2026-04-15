#pragma once
#include "engine/engine.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/components/gameplay_functionality_components/fuel.hpp"
#include "projects/game/components/gameplay_functionality_components/ore_collector.hpp"
#include "projects/game/components/gameplay_functionality_components/upgrade.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

template <typename T>
static Wallet* get_wallet(std::string_view label) {
    const auto view = std::as_const(tmt::engine.ecs).view<T>();
    if (view.empty()) {
        tmt::Log::error("No {} entity found", label);
        return nullptr;
    }
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(view.front().entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component on {} entity", label);
    }
    return wallet;
}

static void apply_multiplier(Currencies& currencies, float multiplier) {
    for (auto& [ore, count] : currencies.resource_counts) {
        count = static_cast<uint64_t>(static_cast<float>(count) * multiplier);
    }
    currencies.dollars = static_cast<uint64_t>(static_cast<float>(currencies.dollars) * multiplier);
}

static void save_resources_on_run(float multiplier_percentage) {
    auto* collector_wallet = get_wallet<OreCollector>("OreCollector");
    if (collector_wallet == nullptr) return;

    auto* player_wallet = get_wallet<Player>("Player");
    if (player_wallet == nullptr) return;

    tmt::Log::info("Penalty of {:.2f}, applied when saving current run resources", multiplier_percentage);

    apply_multiplier(collector_wallet->currencies, multiplier_percentage);
    collector_wallet->currencies.print_values();

    auto& persistent_curr = tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES);
    persistent_curr += collector_wallet->currencies;
    collector_wallet->currencies = Currencies {};

    player_wallet->currencies = persistent_curr;
}
static std::optional<PersistentStats> collect_stats() {
    const auto& ecs = std::as_const(tmt::engine.ecs);

    const auto player_view = ecs.view<Player>();
    if (player_view.empty()) {
        tmt::Log::error("No Player entity found when collecting stats");
        return {};
    }

    const auto weapon_view = ecs.view<Weapon>();
    if (weapon_view.empty()) {
        tmt::Log::error("No Weapon entity found when collecting stats");
        return {};
    }

    const auto barge_view = ecs.view<FuelComponent>();
    if (barge_view.empty()) {
        tmt::Log::error("No barge entity found when collecting stats");
        return {};
    }

    const auto* player = ecs.try_get_component<Player>(player_view.front().entity);
    if (!player) {
        tmt::Log::error("No Player component found when collecting stats");
        return {};
    }

    const auto* weapon = ecs.try_get_component<Weapon>(weapon_view.front().entity);
    if (!weapon) {
        tmt::Log::error("No Weapon component found when collecting stats");
        return {};
    }

    const auto* fuel = ecs.try_get_component<FuelComponent>(barge_view.front().entity);
    if (!fuel) {
        tmt::Log::error("No FuelComponent found when collecting stats");
        return {};
    }

    return PersistentStats {
        .max_health = player->health.max_value,
        .max_battery = player->energy.max_value,
        .max_speed = player->max_speed,
        .acceleration = player->acceleration,
        .primary_atk_speed = weapon->primary_fire_rate.shots_per_second,
        .secondary_atk_speed = weapon->secondary_fire_rate.shots_per_second,
        .barge_max_fuel = fuel->max_fuel,
    };
}

static bool apply_stats(tmt::Entity player_entity, tmt::Entity barge_entity, const PersistentStats& stats) {
    auto* player = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player) {
        tmt::Log::error("No Player component found when applying stats");
        return false;
    }

    auto* weapon = tmt::engine.ecs.try_get_component<Weapon>(player_entity);
    if (!weapon) {
        tmt::Log::error("No Weapon component found when applying stats");
        return false;
    }

    auto* fuel = tmt::engine.ecs.try_get_component<FuelComponent>(barge_entity);
    if (!fuel) {
        tmt::Log::error("No FuelComponent found when applying stats");
        return false;
    }

    player->health.max_value = stats.max_health;
    player->energy.max_value = stats.max_battery;
    player->max_speed = stats.max_speed;
    player->acceleration = stats.acceleration;

    weapon->primary_fire_rate.shots_per_second = stats.primary_atk_speed;
    weapon->secondary_fire_rate.shots_per_second = stats.secondary_atk_speed;

    fuel->max_fuel = stats.barge_max_fuel;

    return true;
}
static void save_after_upgrade() {
    auto* player_wallet = get_wallet<Player>("Player");
    if (player_wallet == nullptr) return;

    auto stats = collect_stats();
    if (!stats.has_value()) return;

    tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) = player_wallet->currencies;
    tmt::engine.player_data.get<PersistentStats>(PERSISTENT_STATS) = *stats;
}

}  // namespace game
