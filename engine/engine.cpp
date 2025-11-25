#include "engine.hpp"

#include <iostream>

#include "core/logger.hpp"
#include "core/window.hpp"

#include "rendering/renderer.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() : window(*new Window()), renderer(*new Renderer()) {}

Engine::~Engine() { 
    delete &window;
    delete &renderer;
}

void Engine::end() { renderer.end(); }

// Example stuff
void Engine::run() {
    PhysicsSystem physics {};
    AnimationSystem animation {};

    physics.update(0.016f);
    animation.update(0.016f);

    while (window.is_running) {
        window.update();
        renderer.update();
    }
}

void Engine::init() {
    window.init();
    renderer.init();

    Log::init();
    tmt::Log::info("No scope here!");
    tmt::Log::warn(Log::LoggerScope::GAME, "Game scope here!");
    tmt::Log::warn(Log::LoggerScope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(Log::LoggerScope::ENGINE, "THERMITE scope here!");
}

}  // namespace tmt
