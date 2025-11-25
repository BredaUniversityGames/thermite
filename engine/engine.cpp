#include "engine.hpp"

#include <iostream>
#include <chrono>

#include "core/logger.hpp"
#include "core/window.hpp"
#include "core/ecs.hpp"
#include "core/timer.hpp"

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

#include "events/engine.hpp"
#include "events/update.hpp"

/* Singleton */
tmt::Engine tmt::engine;

namespace tmt {

Engine::Engine() : window(*new Window()), ecs(*new Ecs()) {}

Engine::~Engine() {
    delete &window;
    delete &ecs;
}

void Engine::init(const ApplicationSpecs& specs) {
    Log::init();
    tmt::Log::info("No scope here!");
    tmt::Log::warn(Log::LoggerScope::GAME, "Game scope here!");
    tmt::Log::warn(Log::LoggerScope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(Log::LoggerScope::ENGINE, "THERMITE scope here!");

    ecs.register_system<Physics>();
    ecs.register_system<Animation>();

    OnEngineInit::dispatch(specs);
}

// Example stuff
void Engine::run() {
    float accumulator = 0.0f;

    Timer timer;
    uint64_t frame_count = 0;
    while (window.is_running) {
        window.update();

        const FrameData frame_data = {
            .delta_time = timer.tick(),
            .fixed_delta_time = Config::fixed_time_step,
            .total_time = timer.elapsed(),
            .frame_count = frame_count
        };

        /* Update */
        OnUpdate::dispatch(frame_data);

        accumulator += frame_data.delta_time;
        while (accumulator >= Config::fixed_time_step) {
            accumulator -= Config::fixed_time_step;

            /* Fixed Update */
            OnFixedUpdate::dispatch(frame_data);
        }

        frame_count++;
    }
}

void Engine::end() { OnEngineEnd::dispatch(); }

}  // namespace tmt
