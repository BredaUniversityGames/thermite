#pragma once
#include "glm/gtc/quaternion.hpp"
#include "engine/systems/physics/physics_voxel_data.hpp"

namespace tmt {

struct Box {
    glm::vec3 vertex[8] {};
};

struct VoxelBody {
    // RigidBody properties
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
        STATIC,
        DYNAMIC,
        WANTS_SLEEP,
        SLEEPING,
    };

    Type type = DYNAMIC;

    const float get_inv_mass() const;
    const glm::mat3 get_inv_world_inertia() const;

    // Collider properties
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;

    PhysicsVoxelData voxels = {};

    const tmt::Box get_local_bounds() const;
    const tmt::Box get_world_bounds() const;
    const std::array<glm::vec3, 3> get_axes() const;
    const std::array<std::pair<glm::vec3, glm::vec3>, 12> get_world_edges() const;
};

}  // namespace tmt