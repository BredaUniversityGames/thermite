#include "voxel_collider.hpp"

const tmt::Box tmt::VoxelCollider::get_local_bounds() const {
    // Define local corners of the box
    const float half_width = width / 2.0f;
    const float half_height = height / 2.0f;
    const float half_depth = depth / 2.0f;

    tmt::Box box = {
        glm::vec3(-half_width, -half_height, -half_depth),  // 0
        glm::vec3(half_width, -half_height, -half_depth),   // 1
        glm::vec3(half_width, half_height, -half_depth),    // 2
        glm::vec3(-half_width, half_height, -half_depth),   // 3
        glm::vec3(-half_width, -half_height, half_depth),   // 4
        glm::vec3(half_width, -half_height, half_depth),    // 5
        glm::vec3(half_width, half_height, half_depth),     // 6
        glm::vec3(-half_width, half_height, half_depth),    // 7
    };

    return box;
}

const std::array<glm::vec3, 2> tmt::VoxelCollider::get_world_aabb(const glm::vec3& position, const glm::quat& rotation)
    const {
    std::array<glm::vec3, 2> aabb;
    aabb[0] = glm::vec3(1e32f);   // min
    aabb[1] = glm::vec3(-1e32f);  // max

    Box box = get_local_bounds();
    for (size_t i = 0; i < 8; i++) {
        box.vertex[i] = (rotation * box.vertex[i]) + position;

        if (box.vertex[i].x < aabb[0].x) aabb[0].x = box.vertex[i].x;
        if (box.vertex[i].y < aabb[0].y) aabb[0].y = box.vertex[i].y;
        if (box.vertex[i].z < aabb[0].z) aabb[0].z = box.vertex[i].z;
        if (box.vertex[i].x > aabb[1].x) aabb[1].x = box.vertex[i].x;
        if (box.vertex[i].y > aabb[1].y) aabb[1].y = box.vertex[i].y;
        if (box.vertex[i].z > aabb[1].z) aabb[1].z = box.vertex[i].z;
    }

    return aabb;
}

const std::array<glm::vec3, 2> tmt::VoxelCollider::get_world_aabb_fast(const glm::vec3& position, const glm::quat& rotation)
    const {
    std::array<glm::vec3, 2> aabb;
    std::array<glm::vec3, 3> axes = get_axes(rotation);

    glm::vec3 world_half_extends = glm::vec3(0);
    for (int i = 0; i < 3; ++i) {
        world_half_extends.x += std::abs(axes[i].x) * (width * 0.5f);
        world_half_extends.y += std::abs(axes[i].y) * (height * 0.5f);
        world_half_extends.z += std::abs(axes[i].z) * (depth * 0.5f);
    }

    aabb[0] = position - world_half_extends;  // min
    aabb[1] = position + world_half_extends;  // max

    return aabb;
}

const std::array<glm::vec3, 3> tmt::VoxelCollider::get_axes(const glm::quat& rotation) const {
    std::array<glm::vec3, 3> axes = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
    };

    for (size_t i = 0; i < 3; i++) axes[i] = glm::normalize(rotation * axes[i]);

    return axes;
}

const std::array<glm::vec3, 3> tmt::VoxelCollider::get_axes(Transform& transform) const {
    const auto rot = transform.get_world_rotation();

    std::array<glm::vec3, 3> axes = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
    };

    for (size_t i = 0; i < 3; i++) axes[i] = glm::normalize(rot * axes[i]);

    return axes;
}

const std::array<glm::vec3, 3> tmt::VoxelCollider::get_axes(const RigidBody& rb) const {
    std::array<glm::vec3, 3> axes = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
    };

    for (size_t i = 0; i < 3; i++) axes[i] = glm::normalize(rb.rotation * axes[i]);

    return axes;
}

const tmt::Box tmt::VoxelCollider::get_world_bounds(Transform& transform) const {
    tmt::Box box = get_local_bounds();
    const auto pos = transform.get_world_position();
    const auto rot = transform.get_world_rotation();

    // Transform vertices to world space
    for (size_t i = 0; i < 8; i++) box.vertex[i] = (rot * box.vertex[i]) + pos;

    return box;
}

const tmt::Box tmt::VoxelCollider::get_world_bounds(const RigidBody& rb) const {
    tmt::Box box = get_local_bounds();

    // Transform vertices to world space
    for (size_t i = 0; i < 8; i++) box.vertex[i] = (rb.rotation * box.vertex[i]) + rb.position;

    return box;
}

const std::array<std::pair<glm::vec3, glm::vec3>, 12> tmt::VoxelCollider::get_world_edges(Transform& transform) const {
    auto box = get_world_bounds(transform);
    return std::array<std::pair<glm::vec3, glm::vec3>, 12> {{
        // X axis edges
        {box.vertex[0], box.vertex[1]},
        {box.vertex[2], box.vertex[3]},
        {box.vertex[4], box.vertex[5]},
        {box.vertex[6], box.vertex[7]},

        // Y axis edges
        {box.vertex[0], box.vertex[3]},
        {box.vertex[1], box.vertex[2]},
        {box.vertex[4], box.vertex[7]},
        {box.vertex[5], box.vertex[6]},

        // Z axis edges
        {box.vertex[0], box.vertex[4]},
        {box.vertex[1], box.vertex[5]},
        {box.vertex[2], box.vertex[6]},
        {box.vertex[3], box.vertex[7]},
    }};
}

const std::array<std::pair<glm::vec3, glm::vec3>, 12> tmt::VoxelCollider::get_world_edges(const RigidBody& rb) const {
    auto box = get_world_bounds(rb);
    return std::array<std::pair<glm::vec3, glm::vec3>, 12> {{
        // X axis edges
        {box.vertex[0], box.vertex[1]},
        {box.vertex[2], box.vertex[3]},
        {box.vertex[4], box.vertex[5]},
        {box.vertex[6], box.vertex[7]},

        // Y axis edges
        {box.vertex[0], box.vertex[3]},
        {box.vertex[1], box.vertex[2]},
        {box.vertex[4], box.vertex[7]},
        {box.vertex[5], box.vertex[6]},

        // Z axis edges
        {box.vertex[0], box.vertex[4]},
        {box.vertex[1], box.vertex[5]},
        {box.vertex[2], box.vertex[6]},
        {box.vertex[3], box.vertex[7]},
    }};
}
