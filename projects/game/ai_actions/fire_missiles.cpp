#include "fire_missiles.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"
// todo: make projects not have to use realtive paths
#include "../components/medium_enemy.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <cstdlib>

void FireMissiles::on_start(tmt::Entity enemy_entity) {
    for (const auto& [CamEntity, camera] : tmt::engine.ecs.view<tmt::Camera>().each()) {
        player = CamEntity;
        break;
    }
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    missiles = enemy.missile_burst;
    burst_interval = enemy.burst_interval;

}

void FireMissiles::on_tick(tmt::Entity enemy_entity, float dt) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);

    tmt::Transform& enemy_transform = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity);
    const auto& enemy_entity_pos = enemy_transform.get_world_position();
    const auto& player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player).get_world_position();

    // Follow path to player
    std::optional<glm::vec3> direction = nav_mesh.follow_path(enemy_entity_pos, player_pos);

    // If path has more waypoints, move to next one
    if (direction) {
        auto velocity = glm::vec3(*direction * enemy.walk_speed);
        enemy.velocity += velocity;
    } else {
        // Close enough to consider node reached, force path recompute
        nav_mesh.path.clear();
    }

    interval_timer += dt;
    if (interval_timer > burst_interval) {
        interval_timer -= burst_interval;
        Missile missile;
        missile.position = enemy_entity_pos;
        missile.max_home_time = enemy.home_duration;
        missile.home_time = enemy.home_duration;
        missile.home_speed = enemy.home_speed;
        missile.max_launch_time = enemy.launch_duration; 
        missile.launch_time = enemy.launch_duration;
        missile.launch_speed = enemy.launch_speed;

        //set random offset
        missile.offset = glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness,
            (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness
        );

        enemy.missiles.emplace_back(missile);
        missiles--;
        
    }
}

bool FireMissiles::is_done(tmt::Entity enemy_entity) const {
    if (missiles == 0) {
        auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
        enemy.missile_timer = interval_timer * enemy.missile_burst;
        return true;
    }
    return false;
}
