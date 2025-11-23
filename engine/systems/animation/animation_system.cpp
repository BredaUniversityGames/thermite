#include "systems/animation/animation_system.hpp"
#include <iostream>

namespace thermite
{
    void AnimationSystem::Initialize()
    {
        std::cout << "[Thermite][Animation] Initialized.\n";
    }

    void AnimationSystem::Update(float deltaTime)
    {
        std::cout << "[Thermite][Animation] Updating with dt = "
            << deltaTime << " seconds.\n";
    }
}