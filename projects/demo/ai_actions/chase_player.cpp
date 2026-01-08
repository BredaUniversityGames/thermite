#include "chase_player.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/walking.hpp"
#include "engine/core/components/camera.hpp"

void ChasePlayer::on_start(tmt::Entity entity, tmt::Registry& registry) {
    //
    for (const auto& [entity, camera] : tmt::engine.ecs.get_registry().view<tmt::Camera>().each()) {
        player = entity;
        break;
    }
}

void ChasePlayer::on_tick(tmt::Entity walking_entity, tmt::Registry& registry, float dt) {
    const auto& walking = tmt::engine.ecs.get_component<Walking>(walking_entity);

    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walking.walkable_asteroid);

    const auto& walking_entity_pos = tmt::engine.ecs.get_component<tmt::Transform>(walking_entity).get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    int start_pos = nav_mesh.find_closest_node(walking_entity_pos);
    int closest_node_id = nav_mesh.find_closest_node(player_pos);

    std::vector<int> path = nav_mesh.find_path(start_pos, closest_node_id);

    if (path.size() > 1) {
        glm::vec3 target_pos = nav_mesh.nodes[path[1]].world_pos;
        tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(walking_entity);
        glm::vec3 direction = glm::normalize(target_pos - walking_transform.get_world_position());
        walking_transform.set_world_position(walking_transform.get_world_position() + direction * walking.walk_speed * dt);
    } else {
        // Reached the player or no path found
        done_walking = true;
    }

    // distance check
    float distance_to_player = glm::length(player_pos - walking_entity_pos);
    if (distance_to_player < walking.attack_range) {
        done_walking = true;
    }
}

bool ChasePlayer::is_done(tmt::Entity /*agent*/, tmt::Registry& /* ecs */) const { return done_walking; }