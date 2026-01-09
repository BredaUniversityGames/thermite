#include "chase_player.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/walking.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"

void ChasePlayer::on_start(tmt::Entity entity, tmt::Registry& registry) {
    //
    for (const auto& [entity, camera] : tmt::engine.ecs.get_registry().view<tmt::Camera>().each()) {
        player = entity;
        break;
    }
}

void ChasePlayer::on_tick(tmt::Entity walking_entity, tmt::Registry&, float dt) {
    const auto& walking = tmt::engine.ecs.get_component<Walking>(walking_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(walking.walkable_asteroid);

    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(walking_entity);
    const auto& walking_entity_pos = walking_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    // Check attack range first
    float distance_to_player = glm::length(player_pos - walking_entity_pos);
    if (distance_to_player < walking.attack_range) {
        done_walking = true;
        return;
    }

    // Calculate path
    int start_pos = nav_mesh.find_closest_node(walking_entity_pos);
    int closest_node_id = nav_mesh.find_closest_node(player_pos);
    std::vector<int> path = nav_mesh.find_path(start_pos, closest_node_id);

    // If path has more waypoints, move to next one
    if (path.size() > 1) {
        glm::vec3 target_pos = nav_mesh.nodes[path[1]].world_pos;

        glm::vec3 current_pos = walking_transform.get_world_position();
        if (glm::distance(target_pos, current_pos) > 0.01f) {
            glm::vec3 direction = glm::normalize(target_pos - current_pos);
            walking_transform.set_world_position(current_pos + direction * walking.walk_speed * dt);
        }
    } else {
        // At the last node or no path - stop moving
        done_walking = true;
        return;
    }

    // Debug visualization
    for (size_t i = 0; i < path.size() - 1; ++i) {
        glm::vec3 from = nav_mesh.nodes[path[i]].world_pos;
        glm::vec3 to = nav_mesh.nodes[path[i + 1]].world_pos;
        tmt::engine.polyline.draw_line(from, to);
    }
}

bool ChasePlayer::is_done(tmt::Entity /*agent*/, tmt::Registry& /* ecs */) const { return done_walking; }