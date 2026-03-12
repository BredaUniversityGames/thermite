#include "ore_manager.hpp"

#include "engine/systems/physics/components/voxel_body.hpp"

namespace game {
void OreManager::start() {
    
}

void OreManager::update(const tmt::FrameData& time) {
    
}

void OreManager::end() {
    
}

void OreManager::trigger_thermite_explosion(tmt::Entity voxel_entity) {
    auto vox_body = tmt::engine.ecs.try_get_component<tmt::VoxelBody>(voxel_entity);
}

}