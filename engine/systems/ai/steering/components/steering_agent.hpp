#pragma once

#include <glm/glm.hpp>

struct SteeringAgent {
    glm::vec3 velocity = {};
    glm::vec3 acceleration = {};

    float max_speed = 5.f;
    float max_force = 10.f;
    float mass = 1.f;

    bool active = true;
};
