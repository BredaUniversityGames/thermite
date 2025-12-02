#pragma once

namespace tmt {

/* Camera component used to describe a view of the scene. */
struct Camera {
    /* Focal length of the camera. */
    float focal_length = 0.1f;
    /* Physical sensor size of the camera. */
    float sensor_size = 0.1f;
    /* True if this camera is active. */
    bool active = true;
};

}  // namespace tmt
