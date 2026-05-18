#include "svh_spawner.hpp"

namespace game {

void SVHSpawner::start() {
    if (!scene_to_spawn) {
        tmt::Log::error("No scene assigned to SVHSpawner on entity {}. Please assign a scene to spawn.", entity);
        return;
    }
    const std::vector<tmt::Entity> spawned_entities = scene_to_spawn->instantiate_entities(entity, false);
    if (spawned_entities.empty()) {
        tmt::Log::error("Failed to spawn scene from SVHSpawner on entity {}. The scene did not instantiate any entities.", entity);
    }
}

void SVHSpawner::update(const tmt::FrameData&) {}

void SVHSpawner::end() {}

}  // namespace game