#pragma once

#include <glm/glm.hpp>

enum class SteeringMode { None, Seek, Arrive, Flee, Wander, FollowPath };

struct SteeringRequest {
    SteeringMode mode = SteeringMode::None;

    glm::vec3 target_position = {};
    float arrive_radius = 0.5f;

    bool completed = false;
};
