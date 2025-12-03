#pragma once

#include "shared/aabb.hpp"

namespace tmt {

/* Represents a renderable voxel object. (used on the GPU) */
struct VoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4 world_to_local {};
    /* Local-space to world-space transformation matrix. */
    glm::mat4 local_to_world {};

    /* Size of the object in voxels. */
    glm::uvec3 size {};

    /* Get the axis-aligned bounding box of this voxel object. */
    inline Aabb aabb() const {
        const glm::vec3 center = glm::vec3(local_to_world[3]);
        const glm::vec3 unit_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
        const glm::vec3 x_extent = glm::abs(glm::vec3(local_to_world[0]) * unit_extent);
        const glm::vec3 y_extent = glm::abs(glm::vec3(local_to_world[1]) * unit_extent);
        const glm::vec3 z_extent = glm::abs(glm::vec3(local_to_world[2]) * unit_extent);
        const glm::vec3 extent = x_extent + y_extent + z_extent;
        return Aabb(center - extent, center + extent);
    }
};

}  // namespace tmt
