#include "fire_laser.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void FireLaser::on_start(tmt::Entity enemy_entity) {
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);

    duration = enemy.laser_duration;
    accuracy = enemy.laser_accuracy;
    accuracy_accum = 0.0f;

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    direction = player_pos - enemy_entity_pos + glm::vec3(
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness,
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.laser_max_randomness
                );
    direction = glm::normalize(direction);
}

void FireLaser::on_tick(tmt::Entity enemy_entity, float dt) {
    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    glm::vec3 target_dir = glm::normalize(player_pos - enemy_entity_pos);
    glm::vec3 axis = glm::cross(direction, target_dir);
    if (glm::length(axis) > 0.001f) {
        axis = glm::normalize(axis);
        float angle = glm::acos(glm::clamp(glm::dot(direction, target_dir), -1.0f, 1.0f));
        float rotate_by = glm::min(angle, accuracy * dt);
        direction = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0f), rotate_by, axis) * glm::vec4(direction, 0.0f)));
    }

    accuracy_accum += dt;

    tmt::engine.polyline.use_line_width(10.0f);
    //tmt::engine.polyline.draw_line(enemy_entity_pos, enemy_entity_pos + direction * 50.0f);
    // draw spheres along the line for better visibility
    for (float i = 0; i < 50.0f; i += 0.1f) {
        tmt::engine.polyline.draw_sphere(enemy_entity_pos + direction * i, .01f);
    }
    duration -= dt;
}

bool FireLaser::is_done(tmt::Entity enemy_entity) const {
    if (duration < 0.0f) {
        auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
        enemy.laser_timer = enemy.laser_duration;
        return true;
    }
    return false;
}