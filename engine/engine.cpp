#include "engine.hpp"
#include "systems/physics/physics_system.hpp"
#include "systems/animation/animation_system.hpp"

#include <iostream>

namespace thermite
{
    Engine::Engine(std::string name)
        : m_name(std::move(name))
    {
    }

    const std::string& Engine::get_name() const
    {
        return m_name;
    }

    //example stuff
    void Engine::run()
    {
        std::cout << "[Thermite] Engine \"" << m_name << "\" starting...\n";

        PhysicsSystem physics;
        AnimationSystem animation;

        physics.Initialize();
        animation.Initialize();

        physics.Update(0.016f);
        animation.Update(0.016f);

        std::cout << "[Thermite] Application \"" << m_name << "\" shutting down.\n";
    }
}