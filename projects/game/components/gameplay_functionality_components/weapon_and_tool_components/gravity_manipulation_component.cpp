#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/polyline.hpp"
#include "weapon.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "engine/systems/ai/steering/components/steering_agent.hpp"

namespace game {

void GravityManipulationComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&GravityManipulationComponent::on_weapon_fired>(this);

    max_mass = tmt::engine.player_data.get<float>(GRAVITY_GUN_DATA, max_mass);
}

void GravityManipulationComponent::update(const tmt::FrameData& time) {
    if (currently_manipulated_entities.empty()) {
        clear_enemy_override();
    }
}

void GravityManipulationComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&GravityManipulationComponent::on_weapon_fired>(this);
}
void GravityManipulationComponent::draw_debug_lines() const {
    if (attraction_point_entity == entt::null) return;
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    tmt::engine.polyline.draw_sphere(attraction_pos, range);
}

void GravityManipulationComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;
    grav_point_check();
    if (e.secondary_shot == false) {
        grav_attract();
    } else {
        grav_shoot();

        // Get the animated rig for the tool animations
        auto* rig_controller = tmt::engine.ecs.try_get_component<tmt::RigController>(animated_tool_entity);
        if (rig_controller) {
            rig_controller->set_parameter_trigger("Shoot");
        }
    }
}

void GravityManipulationComponent::grav_point_check() {
    currently_manipulated_entities.clear();

    // clear_enemy_override();

    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    auto physical_entity_view = tmt::engine.ecs.view<tmt::VoxelBody>();
    for (entt::entity e : physical_entity_view) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);

        // Ignore static bodies unless its enemies (layer 2)
        if (vb.type == tmt::VoxelBody::STATIC && vb.layer != 2) continue;

        if (vb.get_mass() >= max_mass) continue;

        // float distance = glm::distance(vb.center_of_mass, attraction_pos);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(e);

        glm::vec3 world_pos = transform.get_world_position();

        float distance = glm::length(world_pos - attraction_pos);

        if (distance < range) {
            currently_manipulated_entities.push_back(e);
        }
    }

    // apply_enemy_override();
}

void GravityManipulationComponent::grav_attract() {
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    for (auto e : currently_manipulated_entities) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);
        float distance = glm::length(attraction_pos - vb.center_of_mass);
        glm::vec3 direction = glm::normalize(attraction_pos - vb.center_of_mass);
        float dist_factor = glm::clamp(distance / range, 0.0f, 1.0f);
        glm::vec3 target_velocity = direction * pull_strength * dist_factor;
        vb.velocity = glm::mix(vb.velocity, target_velocity, attraction_acceleration);
    }
}

void GravityManipulationComponent::grav_shoot() {
    glm::vec3 direction = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_forward();
    for (auto e : currently_manipulated_entities) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);
        vb.velocity += direction * push_strength;
    }
}

void GravityManipulationComponent::apply_enemy_override() {
    auto attraction_pos = tmt::engine.ecs.get_component<tmt::Transform>(attraction_point_entity).get_world_position();

    for (auto e : currently_manipulated_entities) {
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(e);

        // only enemies
        if (vb.layer != 2) continue;

        auto* agent = tmt::engine.ecs.try_get_component<SteeringAgent>(e);

        if (!agent) continue;

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(e);

        glm::vec3 pos = transform.get_world_position();

        glm::vec3 delta = attraction_pos - pos;

        // safety guard against NaN / zero length
        float len2 = glm::dot(delta, delta);
        if (len2 < 0.000001f) continue;

        float distance = sqrt(len2);

        glm::vec3 direction = delta / distance;

        float dist_factor = glm::clamp(distance / range, 0.0f, 1.0f);

        glm::vec3 force = direction * pull_strength * dist_factor;

        agent->force_override = true;
        agent->override_force = force;
    }
}

void GravityManipulationComponent::clear_enemy_override() {
    for (auto e : currently_manipulated_entities) {
        auto* agent = tmt::engine.ecs.try_get_component<SteeringAgent>(e);

        if (!agent) continue;

        agent->force_override = false;
        agent->override_force = glm::vec3(0);
    }
}

}  // namespace game
