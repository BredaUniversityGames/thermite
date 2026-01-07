#pragma once

#include "shared/aabb.hpp"
#include "shared/ray.hpp"

namespace tmt {

struct Ray;

/* Represents a renderable voxel object. (used on the GPU) */
struct alignas(16) VoxelObject {
    /* World-space to local-space transformation matrix. */
    glm::mat4 world_to_local {};
    /* Local-space to world-space transformation matrix. */
    glm::mat4 local_to_world {};

    /* Size of the object in voxels. */
    glm::uvec3 size {};
    /* Reciprocal of the width of the voxel tree structure. */
    float rcp_tree_width = 0.0f;
    /* Depth of the voxel tree structure. */
    uint32_t tree_depth = 0u;

    /* Bindless resource handles. */
    uint32_t blas_handle = 0xFFFFFFFFu;
    uint32_t voxels_handle = 0xFFFFFFFFu;
    uint32_t palette_handle = 0xFFFFFFFFu;

    /* Get the axis-aligned bounding box of this voxel object. */
    inline Aabb aabb() const {
        const glm::vec3 center = glm::vec3(local_to_world[3]);
        const glm::vec3 unit_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
        const glm::vec3 extent = glm::mat3(glm::abs(glm::vec3(local_to_world[0])), glm::abs(glm::vec3(local_to_world[1])), glm::abs(glm::vec3(local_to_world[2]))) * unit_extent;
        return Aabb(center - extent, center + extent);
    }

    /* Intersect the voxel object with a ray. */
    Hit intersect(const Ray& ray, const float tmax) const {
        // TODO: Implement the ray intersection function!

        /* Transform the ray into the local space of the object */
        const glm::vec3 local_origin = glm::vec3(world_to_local * glm::vec4(ray.origin, 1.0f));
        const glm::vec3 local_dir = glm::vec3(world_to_local * glm::vec4(ray.dir, 0.0f));
        const glm::vec3 local_rcp_dir = 1.0f / local_dir;

        /* Intersect the object AABB in local-space */
        const glm::vec3 half_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
        const glm::vec3 t_to_min = (-half_extent - local_origin) * local_rcp_dir;
        const glm::vec3 t_to_max = (half_extent - local_origin) * local_rcp_dir;
        const glm::vec3 t_min = glm::min(t_to_min, t_to_max);
        const glm::vec3 t_max = glm::max(t_to_min, t_to_max);
        const float t_near = glm::max(glm::max(glm::max(t_min.x, t_min.y), t_min.z), 0.0f);
        const float t_far = glm::min(glm::min(glm::min(t_max.x, t_max.y), t_max.z), tmax);
        if (t_near > t_far) return Hit(); /* miss */

        return Hit(t_near, 0xFFFFFFFFu);
    }
};

}  // namespace tmt
