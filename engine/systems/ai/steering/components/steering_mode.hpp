#pragma once

#include <glm/glm.hpp>

enum class SteeringMode { NONE, SEEK, ARRIVE, FLEE, WANDER };

struct WanderData {
    glm::vec3 wanderTarget = glm::vec3(0.0f);
    float wanderRadius = 1.5f;    // radius of jitter circle
    float wanderDistance = 3.0f;  // distance in front of agent
    float wanderJitter = 0.5f;    // max random jitter per second
};

struct SteeringRequest {
    SteeringMode mode = SteeringMode::NONE;

    glm::vec3 target_position = {};
    float arrive_radius = 0.5f;

    bool completed = false;

    WanderData wanderData;
};
