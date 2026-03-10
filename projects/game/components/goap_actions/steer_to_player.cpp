#include "steer_to_player.hpp"
#include "steer_to_player.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "../player.hpp"

namespace game {

void SteerToPlayer::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!tmt::engine.ecs.valid(player_entity)) {
        // Create or reset steering request
        SteeringRequest request {};
        request.mode = SteeringMode::ARRIVE;

        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game

        auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
        request.target_position = player_transform->get_world_position();

        registry.emplace_or_replace<SteeringRequest>(agent, request);
    }
}

void SteerToPlayer::on_tick(tmt::Entity agent, float /*dt*/) {
    if (!tmt::engine.ecs.valid(player_entity)) return;

    auto& registry = tmt::engine.ecs.get_registry();
    auto& request = registry.get<SteeringRequest>(agent);

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    auto* agent_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(agent);
    if (!player_transform || !agent_transform) return;

    glm::vec3 player_pos = player_transform->get_world_position();
    glm::vec3 agent_pos = agent_transform->get_world_position();
    glm::vec3 to_player = player_pos - agent_pos;

    if (glm::length(to_player) < 0.01f) return;

    // Clamp stop offset so we don't pass the agent
    float dist_to_player = glm::length(to_player);
    float safe_offset = glm::min(request.explosion_range, dist_to_player - 0.1f);
    safe_offset = glm::max(0.f, safe_offset);
    glm::vec3 stop_offset = glm::normalize(to_player) * safe_offset;

    glm::vec3 desired_target = player_pos - stop_offset;

    // Update request only if it changed significantly
    glm::vec3 diff = request.target_position - desired_target;
    if (glm::dot(diff, diff) > 0.0001f) {
        request.target_position = desired_target;
        request.arrive_radius = 0.5f;
    }
}

bool SteerToPlayer::is_done(tmt::Entity agent) const {
    auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<SteeringRequest>(agent)) return true;

    auto& request = registry.get<SteeringRequest>(agent);
    return request.completed;
}

void SteerToPlayer::on_finished(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();
    if (registry.any_of<SteeringRequest>(agent)) {
        registry.remove<SteeringRequest>(agent);
    }
}

void SteerToPlayer::on_interrupt(tmt::Entity agent) {
    on_finished(agent);
}

}  // namespace game
