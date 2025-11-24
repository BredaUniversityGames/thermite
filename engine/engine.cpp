#include "engine.hpp"

#include <iostream>

#include <SDL3/SDL.h>

#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"


namespace thermite {

static SDL_Window* window = nullptr;

Engine::Engine(std::string name) : name(std::move(name)) {
    SDL_SetAppMetadata(
        "Thermite Engine", "0.1", "com.example.renderer-clear"
    );

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    }

    if (!SDL_CreateWindow("Thermite", 1920, 1080, SDL_WINDOW_VULKAN)) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
    }
}

const std::string& Engine::get_name() const { return name; }

// Example stuff
void Engine::run() {
    std::cout << "[Thermite] Engine \"" << name << "\" starting...\n";

    PhysicsSystem physics {};
    AnimationSystem animation {};

    physics.init();
    animation.init();

    physics.update(0.016f);
    animation.update(0.016f);
    
    SDL_Event Event;

    bool IsRunning = true;
    while (IsRunning) {
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_EVENT_QUIT) {
                IsRunning = false;
            }
        }
    }

    std::cout << "[Thermite] Application \"" << name << "\" shutting down.\n";
}

}  // namespace thermite
