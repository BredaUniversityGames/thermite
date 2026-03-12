#include "steer_to_player.hpp"
#include "steer_to_player.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "../player.hpp"

namespace game {

void SteerToPlayer::on_start(tmt::Entity agent) {
    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;
    }

    SteeringRequest request {};
    request.mode = SteeringMode::ARRIVE;

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    if (player_transform) {
        request.target_position = player_transform->get_world_position();
    }

    auto& registry = tmt::engine.ecs.get_registry();
    registry.emplace_or_replace<SteeringRequest>(agent, request);
}

void SteerToPlayer::on_fixed_tick(tmt::Entity agent, float /*dt*/) {
    if (!tmt::engine.ecs.valid(player_entity)) return;

    auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<SteeringRequest>(agent)) return;
    auto& request = registry.get<SteeringRequest>(agent);

    auto* player_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(player_entity);
    auto* agent_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(agent);
    if (!player_transform || !agent_transform) return;

    glm::vec3 player_pos = player_transform->get_world_position();
    glm::vec3 agent_pos = agent_transform->get_world_position();
    glm::vec3 to_player = player_pos - agent_pos;

    float distance = glm::length(to_player);

    // update world state
    auto* ws = tmt::engine.ecs.try_get_component<tmt::WorldState>(agent);
    if (ws) {
        uint32_t fact = (uint32_t)std::hash<std::string>()("player_in_range");
        ws->facts[fact] = distance <= request.activation_range;
    }

    done = distance <= request.min_explosion_range;

    if (distance < 0.01f) return;

    // Always steer toward the player
    glm::vec3 diff = request.target_position - player_pos;

    if (glm::dot(diff, diff) > 0.0001f) {
        request.target_position = player_pos;
    }
}

bool SteerToPlayer::is_done(tmt::Entity agent) const {
    /*auto& registry = tmt::engine.ecs.get_registry();
    if (!registry.any_of<SteeringRequest>(agent)) return true;

    auto& request = registry.get<SteeringRequest>(agent);
    return request.completed;*/
    return done;
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
