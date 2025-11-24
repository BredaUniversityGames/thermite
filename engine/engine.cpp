#include "engine.hpp"

#include <iostream>

#include "core/window.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() : window(*new Window()) {}

Engine::~Engine() { delete &window; }

void Engine::init() { window.init(); }

void Engine::end() {}

// Example stuff
void Engine::run() {
    PhysicsSystem physics {};
    AnimationSystem animation {};

    physics.init();
    animation.init();

    physics.update(0.016f);
    animation.update(0.016f);

    while (window.is_running) {
        window.update();
    }
}

}  // namespace tmt
