#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
#include "player.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "weapon.hpp"

namespace game {
void GravityManipulationComponent::start() {
    
}

void GravityManipulationComponent::update(const tmt::FrameData& time) {
    
}

void GravityManipulationComponent::end() {
    
}

void GravityManipulationComponent::grav_point() {

    std::vector<tmt::Entity> entities_in_radius;
    std::vector<tmt::Entity> manipulated_entities;
    entt::entity player_entity = tmt::engine.ecs.get_registry().view<game::Player, tmt::Camera>().front();
    game::Player player_component = tmt::engine.ecs.get_component<game::Player>(player_entity);

    auto physical_entity_view = tmt::engine.ecs.get_registry().view<tmt::VoxelBody>();

    for (entt::entity entity_vb_view : physical_entity_view) {
        tmt::VoxelBody& current_vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(entity_vb_view);
        // First filter for max mass
        if (current_vb.get_mass() < max_mass) {
            glm::vec3 attract_point = player_component.get_transform().get_world_position();
            attract_point *= glm::vec3(1, 1, 1);

            float distance = glm::distance(current_vb.position, attract_point);
        }
    }
    // TODO: Filter for manipulatable objects -> not possible at the moment

}

void GravityManipulationComponent::grav_shoot() {
    
}
}
