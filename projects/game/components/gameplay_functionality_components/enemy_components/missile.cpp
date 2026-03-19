#include "missile.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/navigation/nav_mesh.hpp"

void game::Missile::start() {
    auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity);
    name.name = "Missile";

    game::MediumEnemy& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    glm::vec3 enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    glm::vec3 missile_pos = enemy_pos;
    if (tmt::engine.ecs.valid(enemy.missile_origin)) {
        tmt::Transform& missile_origin = tmt::engine.ecs.get_component<tmt::Transform>(enemy.missile_origin);
        missile_pos = missile_origin.get_world_position();
    }

    auto& missile_trans = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    missile_trans.set_world_position(missile_pos);

    // set random offset
    offset = glm::vec3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * enemy.missile_max_randomness
    );
}

void game::Missile::update(const tmt::FrameData& time) {
    auto& enemy = tmt::engine.ecs.get_component<game::MediumEnemy>(enemy_entity);
    if (life_time > enemy.life_time) {
        tmt::engine.ecs.remove_component<Missile>(entity);
        return;
    }
    life_time += time.delta_time;

    auto& position = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto player_pos = tmt::engine.ecs.get_component<tmt::Transform>(player_entity).get_world_position();
    auto enemy_pos = tmt::engine.ecs.get_component<tmt::Transform>(enemy_entity).get_world_position();
    auto& nav_mesh = tmt::engine.ecs.get_component<tmt::NavMesh>(enemy.walkable_asteroid);
    auto ground_up = (*nav_mesh.nodes_mesh)[nav_mesh.find_closest_node(enemy_pos)].normal;

    tmt::engine.polyline.draw_sphere(position.get_world_position(), 0.1f);

    // fades from full launch down to zero
    float launch_t = glm::clamp((life_time / enemy.stop_launching_after), 0.0f, 1.0f);
    glm::vec3 launch_contribution = glm::mix(ground_up * enemy.launch_speed, glm::vec3(0.0f), launch_t);

    // fades from zero up to full homing speed
    glm::vec3 home_contribution(0.0f);
    if (life_time > enemy.start_homing_after) {
        float max_home_time = enemy.life_time - enemy.start_homing_after;
        float home_time = life_time - enemy.start_homing_after;
        float home_t = glm::clamp(home_time / max_home_time, 0.0f, 1.0f);  // was inverted

        glm::vec3 to_player = player_pos - position.get_world_position();
        if (glm::dot(to_player, to_player) > 0.0001f) {
            home_contribution = glm::normalize(to_player) * enemy.home_speed * home_t;
        }
    }

    auto added_vel = launch_contribution + home_contribution;
    float length = glm::length(added_vel);
    if (length > 0.0001f && life_time < enemy.start_homing_after) {
        added_vel += offset;
        added_vel = glm::normalize(added_vel) * length;
    }
    velocity += added_vel;
    velocity *= 0.7f;
    position.translate(velocity * time.delta_time);
}

void game::Missile::end() {}