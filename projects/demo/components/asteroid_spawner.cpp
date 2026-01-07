#include "asteroid_spawner.hpp"
#include <cstdlib>

void AsteroidSpawner::start() {
	srand(seed); }

void AsteroidSpawner::update(const tmt::FrameData& time) {}

void AsteroidSpawner::end() {}

void AsteroidSpawner::spawn_asteroid() {

}

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
void AsteroidSpawner::on_inspect() {
    if (ImGui::Button("Spawn Asteroids")) {
        spawn_asteroid();
    }
}
#else
void AsteroidSpawner::on_inspect() {}
#endif