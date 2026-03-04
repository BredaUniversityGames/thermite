#pragma once

#include <glm/glm.hpp>

struct SteeringAgent {
    float max_speed = 5.f;
    float max_force = 20.f;
    bool active = true;
};
