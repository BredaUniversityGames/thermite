#pragma once
#include <ImReflect_macro.hpp>

#include "engine/core/entity.hpp"

namespace tmt {

/* Camera component used to describe a view of the scene. */
struct Camera {
    /* Field of View in degrees. */
    float fov = 80.0f;

    float pitch = 0.0f;
    float yaw = 90.0f;

    /* True if this camera is active. */
    bool active = true;

    static Entity get_active_camera();
    static void set_active_camera(Entity camera_entity);
};

}  // namespace tmt

IMGUI_REFLECT(tmt::Camera, fov, pitch, yaw, active);