#include "svh_spawner.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/components/destructable.hpp"

namespace game {

void SVHSpawner::start() {
    if (!scene_to_spawn) {
        tmt::Log::error("No scene assigned to SVHSpawner on entity {}. Please assign a scene to spawn.", entity);
        return;
    }
    const std::vector<tmt::Entity> roots = scene_to_spawn->instantiate_entities(entity, false);
    if (roots.empty()) {
        tmt::Log::error("Failed to spawn scene from SVHSpawner on entity {}. The scene did not instantiate any entities.", entity);
    }

    for (const tmt::Entity root : roots) {
        init_children(root);
    }
}

void SVHSpawner::update(const tmt::FrameData&) {}

void SVHSpawner::end() {}

void SVHSpawner::init_children(const tmt::Entity parent) {
    const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(parent);
    const auto children = transform.get_children();
    for (const auto child : children) {
        init_children(child);
        /* Add voxel body */
        tmt::engine.ecs.add_component<tmt::VoxelBody>(child);
        tmt::engine.ecs.add_component<tmt::Destructible>(child);
    }
}

}  // namespace game