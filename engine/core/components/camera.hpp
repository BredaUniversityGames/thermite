#pragma once

namespace tmt {

/* Camera component used to describe a view of the scene. */
struct Camera {
    /* World-space to view-space transformation matrix. */
    glm::mat4x4 world_to_view {};

    float focal_length = 0.1f;
    glm::vec2 sensor_size = glm::vec2(0.1f, 0.1f);
};

}  // namespace tmt
