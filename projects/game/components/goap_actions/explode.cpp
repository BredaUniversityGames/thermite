#include "explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "../player.hpp"

namespace game {

void Explode::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game}
    }
}

void Explode::on_tick(tmt::Entity agent, float) {
    if (!tmt::engine.ecs.valid(player_entity)) return;
    auto& registry = tmt::engine.ecs.get_registry();

    auto player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    auto agent_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(agent);

    if (player_transform && agent_transform) {
        float distance = glm::distance(agent_transform->get_world_position(), player_transform->get_world_position());

        auto& request = registry.get<SteeringRequest>(agent);

        auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(agent);
        if (ws) {
            uint32_t fact = (uint32_t)std::hash<std::string>()("at_target");
            ws->facts[fact] = distance <= request.max_explosion_range;
        }
    }
}

bool Explode::is_done(tmt::Entity agent) const {
    return false;
}

void Explode::on_finished(tmt::Entity agent) {}

void Explode::on_interrupt(tmt::Entity agent) {}

}  // namespace game
