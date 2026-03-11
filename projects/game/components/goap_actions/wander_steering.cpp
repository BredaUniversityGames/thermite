#include "wander_steering.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "../player.hpp"

namespace game {

void WanderSteering::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    // Create a wander steering request
    SteeringRequest request {};
    request.mode = SteeringMode::WANDER;

    // set initial WanderData
    request.wander_data.wander_radius = 2.0f;                // radius of the wander sphere
    request.wander_data.wander_distance = 3.0f;              // distance in front of the agent
    request.wander_data.wander_jitter = 0.5f;                // how much the target jitters per second
    request.wander_data.wander_target = glm::vec3(0, 0, 0);  // start at sphere center

    registry.emplace_or_replace<SteeringRequest>(agent, request);

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    }
}

void WanderSteering::on_tick(tmt::Entity agent, float /*dt*/) {
    if (!tmt::engine.ecs.valid(player_entity)) return;
    auto& registry = tmt::engine.ecs.get_registry();

    auto player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    auto agent_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(agent);

    if (player_transform && agent_transform) {
        float distance = glm::distance(agent_transform->get_world_position(), player_transform->get_world_position());

        auto& request = registry.get<SteeringRequest>(agent);
        auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(agent);
        if (ws) {
            uint32_t fact = (uint32_t)std::hash<std::string>()("player_in_range");
            ws->facts[fact] = distance <= request.activation_range;
        }
    }
}

bool WanderSteering::is_done(tmt::Entity agent) const {
    return false;
}

void WanderSteering::on_finished(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();
    if (registry.any_of<SteeringRequest>(agent)) {
        registry.remove<SteeringRequest>(agent);
    }
}

void WanderSteering::on_interrupt(tmt::Entity agent) {
    on_finished(agent);
}

}  // namespace game
