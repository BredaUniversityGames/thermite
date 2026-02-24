#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
#include "player.hpp"

namespace game {

void GravityManipulationComponent::start() {}

void GravityManipulationComponent::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;
    if (input.is_action_pressed(action::SHOOT)) {
        // if mb1 down
        grav_point_check();
        grav_attract();
    }
    if (input.is_action_pressed(action::SECONDARY_TOOL_USE)) {
        // if mb2 down
        grav_point_check();
        grav_shoot();
    }
}

void GravityManipulationComponent::end() {}

void GravityManipulationComponent::grav_point_check() {
    calculate_attract_point();
    currently_manipulated_entities.clear();
    auto physical_entity_view = tmt::engine.ecs.get_registry().view<tmt::VoxelBody>();

    for (entt::entity entity_vb_view : physical_entity_view) {
        tmt::VoxelBody& current_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity_vb_view);
        // First filter for max mass
        if (current_vb.get_mass() < max_mass) {
            float distance = glm::distance(current_vb.position, attract_point);
            // Filter for distance
            if (distance < range) {
                currently_manipulated_entities.push_back(entity_vb_view);
            }
        }
    }
}

void GravityManipulationComponent::calculate_attract_point() {
    auto player_view = tmt::engine.ecs.get_registry().view<Player>();
    if (player_view.begin() == player_view.end()) {
        tmt::Log::error("No Player found.");
        return;
    }
    entt::entity player_entity = player_view.front();
    game::Player player_component = tmt::engine.ecs.get_component<game::Player>(player_entity);

    attract_point = player_component.get_transform().get_world_position();
    auto pitch = player_component.get_camera().pitch;
    auto yaw = player_component.get_camera().yaw;

    // Calculate player looking direction vector from pitch and yaw
    glm::vec3 direction = glm::vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));

    // Calculate the up vector from the player pov
    glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
    glm::vec3 local_up = glm::cross(right, direction);

    attract_point += direction * forward_offset;
    attract_point += local_up * vertical_offset;
}

void GravityManipulationComponent::grav_attract() {
    for (auto manip_entity : currently_manipulated_entities) {
        auto& curr_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(manip_entity);
        curr_vb.velocity += glm::normalize(attract_point - curr_vb.position) * pull_strength;
    }
}

void GravityManipulationComponent::grav_shoot() {
    auto player_view = tmt::engine.ecs.get_registry().view<game::Player>();
    if (player_view.begin() == player_view.end()) {
        tmt::Log::error("No Player found in scene!");
        return;
    }
    entt::entity player_entity = player_view.front();
    game::Player player_component = tmt::engine.ecs.get_component<game::Player>(player_entity);
    auto pitch = player_component.get_camera().pitch;
    auto yaw = player_component.get_camera().yaw;

    // Calculate player looking direction vector from pitch and yaw
    glm::vec3 direction = glm::vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));

    for (auto manip_entity : currently_manipulated_entities) {
        auto& curr_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(manip_entity);
        curr_vb.velocity += direction * push_strength;
    }
}

}  // namespace game
