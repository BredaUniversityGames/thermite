#pragma once

#include "engine/core/renderer/render_view.hpp"

namespace tmt {

/* Ray definition. */
struct Ray {
    glm::vec3 origin {};
    glm::vec3 dir {};
    glm::vec3 rcp_dir {};
};

/* Ray hit data. */
struct Hit {
    float distance = 1e30f;
    uint32_t index = 0xFFFFFFFFu;
    Hit() = default;
    Hit(float t, uint32_t i) : distance(t), index(i) {};
};

/* Convert a pixel coordinate to a normalized device coordinate. */
inline glm::vec2 pixel_to_ndc(glm::ivec2 pixel, glm::uvec2 resolution) { return ((glm::vec2(pixel) + 0.5f) / glm::vec2(resolution)) * 2.0f - 1.0f; }

/* Generate a ray for a pin hole camera. */
inline Ray pinhole_ray(const RenderView& view, glm::ivec2 pixel) {
    const glm::vec2 ndc = pixel_to_ndc(pixel, view.gpu_view.resolution);

    glm::vec4 world_pos = view.gpu_view.clip_to_world * glm::vec4(ndc, 1.0f, 1.0f);
    world_pos = glm::vec4(glm::vec3(world_pos) / world_pos.w, world_pos.w);
    const glm::vec3 world_dir = glm::normalize(glm::vec3(world_pos) - glm::vec3(view.gpu_view.origin));

    Ray ray; /* Fill in the ray struct */
    ray.origin = glm::vec3(view.gpu_view.origin);
    ray.dir = world_dir;
    ray.rcp_dir = glm::clamp(1.0f / ray.dir, -1e30f, 1e30f);
    return ray;
}

}  // namespace tmt
