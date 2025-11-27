#pragma once
#include "glm/gtc/quaternion.hpp"

namespace tmt {

struct RigidBody {
    float accumulated_forces = 1000.0f;
    glm::vec3 stored_velocity = glm::vec3(0);
    glm::vec3 stored_torque = glm::vec3(0);

    float inv_mass = -1.0f;
    glm::mat3 inv_inertia {};

    glm::vec3 com_local_offset = glm::vec3(0);
    glm::vec3 center_of_mass = glm::vec3(0);

    glm::vec3 velocity = glm::vec3(0);
    float linear_drag = 0.3f;
    glm::vec3 angular_velocity = glm::vec3(0);
    float angular_drag = 0.6f;

    float gravity = 9.8f;
    float density = 1.0f;

    glm::vec3 position = glm::vec3(0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);

    enum Type {
        Static,
        Dynamic,
        Wants_Sleep,
        Sleeping,
    };

    Type type = Dynamic;
};

}  // namespace tmt