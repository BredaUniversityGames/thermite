#include "core/Application.hpp"
#include "systems/physics/PhysicsSystem.hpp"
#include "systems/animation/AnimationSystem.hpp"

#include <iostream>

namespace thermite
{
    Application::Application(std::string name)
        : m_name(std::move(name))
    {
    }

    const std::string& Application::GetName() const
    {
        return m_name;
    }

    void Application::Run()
    {
        std::cout << "[Thermite] Application \"" << m_name << "\" starting...\n";

        PhysicsSystem physics;
        AnimationSystem animation;

        physics.Initialize();
        animation.Initialize();

        physics.Update(0.016f);
        animation.Update(0.016f);

        std::cout << "[Thermite] Application \"" << m_name << "\" shutting down.\n";
    }
}