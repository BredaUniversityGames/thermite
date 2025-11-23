#include "systems/physics/physics_system.hpp" 
#include <iostream>

namespace thermite
{
    void PhysicsSystem::Initialize()
    {
        std::cout << "[Thermite][Physics] Initialized.\n";
    }

    void PhysicsSystem::Update(float deltaTime)
    {
        std::cout << "[Thermite][Physics] Updating with dt = "
            << deltaTime << " seconds.\n";
    }
}