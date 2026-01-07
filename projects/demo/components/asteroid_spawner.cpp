#include "asteroid_spawner.hpp"

#include <cstdlib>

#include "engine/core/components/voxel_renderer.hpp"

void AsteroidSpawner::start() {
	srand(seed); }

void AsteroidSpawner::update(const tmt::FrameData& time) {}

void AsteroidSpawner::end() {}

void AsteroidSpawner::spawn_asteroids() {
    create_asteroid();
}

tmt::Transform& AsteroidSpawner::create_asteroid() {
    const tmt::Entity entity = tmt::engine.ecs.create_entity();

    tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    int random_index = rand() % asteroid_bodies.size();
    const auto& asteroid_body = asteroid_bodies.at(random_index);
    renderer.resource = asteroid_body;

    tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    return transform;
}

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
void AsteroidSpawner::on_inspect() {
    if (ImGui::Button("Spawn Asteroids")) {
        spawn_asteroids();
    }
}
#else
void AsteroidSpawner::on_inspect() {}
#endif