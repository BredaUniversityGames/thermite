#include "gravity_manipulation_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "game_input.hpp"
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
    // Check for voxel objects in radius
    auto physical_entity_view = tmt::Engine::ecs.get_registry().view<tmt::VoxelBody>();

    // TODO: Filter for manipulatable objects -> not possible at the moment

}

void GravityManipulationComponent::grav_shoot() {
    
}
}
