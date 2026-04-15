#include "explode.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"

#include "../gameplay_functionality_components/player.hpp"
#include "../managers/ore_manager.hpp"
#include "../gameplay_functionality_components/enemy_components/small_enemy.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace game {

void Explode::on_start(tmt::Entity agent) {
    auto& registry = tmt::engine.ecs.get_registry();

    if (!registry.any_of<SteeringAgent>(agent)) {
        registry.emplace<SteeringAgent>(agent);
    }

    auto* steering = tmt::engine.ecs.systems.try_get<tmt::SteeringSystem>();

    if (!steering) {
        tmt::Log::warn("Steering system not active.");
        return;
    }

    // Get the SteeringAgent component & copy global steering params into this agent
    auto& steering_agent = registry.get<SteeringAgent>(agent);
    steering_agent.params = &steering->overrides().params;

    if (!tmt::engine.ecs.valid(player_entity)) {
        player_entity = tmt::engine.ecs.view<Player>().front().entity;  // Assuming there's only one player entity in the game
    }

    live_charge_time = 0.0f;
    exploded = false;
}

void Explode::on_tick(tmt::Entity agent, float dt) {
    live_charge_time += dt;

    auto& registry = tmt::engine.ecs.get_registry();
    auto& steering_agent = registry.get<SteeringAgent>(agent);

    auto& transform = registry.get<tmt::Transform>(agent);

    glm::vec3 enemy_pos = transform.get_world_position();
    glm::vec3 player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player_entity).get_world_position();

    // direction TO player
    glm::vec3 dir = player_pos - enemy_pos;
    dir.y = 0.0f;

    if (glm::length2(dir) > 0.0001f) {
        dir = glm::normalize(dir);

        glm::quat target_rot = glm::quatLookAt(-dir, glm::vec3(0, 1, 0));

        // optional model correction (VERY likely needed in your engine)
        target_rot *= glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));

        // choose correct rotation owner
        if (registry.any_of<tmt::VoxelBody>(agent)) {
            auto& body = registry.get<tmt::VoxelBody>(agent);

            glm::quat current_rot = body.rotation;
            float turn_speed = 6.0f;

            body.rotation = glm::slerp(current_rot, target_rot, turn_speed * dt);
        } else {
            glm::quat current_rot = transform.get_world_rotation();
            float turn_speed = 6.0f;

            transform.set_world_rotation(glm::slerp(current_rot, target_rot, turn_speed * dt));
        }
    }

    if (live_charge_time >= steering_agent.params->charge_time && !exploded) {
        auto* small_enemy = tmt::engine.ecs.try_get_component<SmallEnemy>(agent);

        // try get small enemy component, and the core entity
        if (!small_enemy || !tmt::engine.ecs.valid(small_enemy->thermite)) {
            tmt::Log::warn("Enemy doesnt have small enemy component OR a core assigned.");
            return;  // nothing to explode
        }

        // try get small enemy component, and the explosion entity, to get the particle emitter
        if (tmt::engine.ecs.valid(small_enemy->explosion)) {
            auto& particles = tmt::engine.ecs.get_component<tmt::ParticleEmitter>(small_enemy->explosion);

            particles.should_burst = true;
        }

        tmt::Entity thermite = small_enemy->thermite;

        // get core entity transform world position
        auto& core_transform = tmt::engine.ecs.get_component<tmt::Transform>(thermite);
        glm::vec3 explosion_center = core_transform.get_world_position();

        // Get everything (with voxel body) within radius and push away
        float radius = steering_agent.params->explosion_range;
        float force = steering_agent.params->explosion_force;  // add this param!

        for (auto [entity, body, transform] : tmt::engine.ecs.view<tmt::VoxelBody, tmt::Transform>().each()) {
            // Only affect dynamic bodies ?
            // if (body.type != tmt::VoxelBody::DYNAMIC) continue;

            glm::vec3 pos = transform.get_world_position();
            glm::vec3 dir = pos - explosion_center;

            float dist = glm::length(dir);

            if (dist <= radius && dist > 0.001f) {
                glm::vec3 normal = glm::normalize(dir);

                // falloff, less force further away
                float strength = 1.0f - (dist / radius);

                glm::vec3 explosion_velocity = normal * force * strength;

                // apply an impulse
                body.velocity += explosion_velocity;
            }
        }

        if (tmt::engine.ecs.valid(player_entity)) {
            auto& player_transform = tmt::engine.ecs.get_component<tmt::Transform>(player_entity);
            auto& player = tmt::engine.ecs.get_component<Player>(player_entity);

            glm::vec3 player_pos = player_transform.get_world_position();
            glm::vec3 dir = player_pos - explosion_center;

            float dist = glm::length(dir);

            if (dist <= radius && dist > 0.001f) {
                // damage
                player.health.value -= steering_agent.params->explosion_damage;

                glm::vec3 normal = glm::normalize(dir);

                // same falloff as physics bodies
                float strength = 1.0f - (dist / radius);

                // push player
                float player_force = steering_agent.params->explosion_force;
                player.apply_impulse(normal, player_force * strength);
            }
        }

        // destroy core entity & explosion
        tmt::engine.ecs.destroy_entity(thermite);
        // tmt::engine.ecs.destroy_entity(small_enemy->explosion);

        // remove GOAP so it doesn't keep acting
        tmt::engine.ecs.remove_component<tmt::GoapAgent>(agent);

        exploded = true;
    }
}

bool Explode::is_done(tmt::Entity agent) const {
    return exploded;
}

void Explode::on_finished(tmt::Entity agent) {}

void Explode::on_interrupt(tmt::Entity agent) {}

}  // namespace game
