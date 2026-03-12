#include "ore_manager.hpp"

#include <queue>

#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

namespace game {
void OreManager::start() {
    
}

void OreManager::update(const tmt::FrameData& time) {

    if (thermite_ore_explosion_cooldown_timer <= 0.0f) {
        for (auto explosion_center : positions_to_explode) {
            for (auto hit_entity : hit_entities) {
                process_thermite_ore_explosion(hit_entity, explosion_center);
            }
        }
        //update explosion list
        positions_to_explode = new_positions_to_explode;
    }

}

void OreManager::end() {
    
}

void OreManager::initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position) {
    hit_entities.insert(voxel_entity);
    positions_to_explode.insert(voxel_position);
}

void OreManager::process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::vec3 explosion_center) {
    auto vox_renderer = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(voxel_entity);
    int voxel_level_radius = static_cast<int>(thermite_ore_explosion_radius) * static_cast<int>(VOXELS_PER_UNIT);

    for (int x = explosion_center.x - voxel_level_radius; x <= explosion_center.x + voxel_level_radius; x++) {
        for (int y = explosion_center.y - voxel_level_radius; y <= explosion_center.y + voxel_level_radius; y++) {
            for (int z = explosion_center.z - voxel_level_radius; z <= explosion_center.z + voxel_level_radius; z++) {
                // distances
                int dx = x - explosion_center.x;
                int dy = y - explosion_center.y;
                int dz = z - explosion_center.z;
                int distance = static_cast<int>(std::sqrt(dx * dx + dy * dy + dz * dz));

                if (distance <= voxel_level_radius) {
                    // add if is thermite to thermite_positions

                    // delete
                }
            }
        }
    }

}
}