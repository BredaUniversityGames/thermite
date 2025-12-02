#pragma once

namespace tmt {

/* Represents a renderable view in the scene. (used on the GPU) */
struct RenderView {
    /* World-space to view-space transformation matrix. */
    glm::mat4x4 world_to_view {};
    /* Origin of the view in world-space. */
    glm::vec3 origin {};
    /* Focal length of the view. */
    float focal_length = 0.0f;
    /* Resolution of the view in pixels. */
    glm::uvec2 resolution {};
    /* Physical sensor size of the view. */
    float sensor_size = 0.0f;
};

}  // namespace tmt
