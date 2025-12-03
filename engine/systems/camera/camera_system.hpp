#pragma once

#include <string>

#include "core/system.hpp"

namespace tmt {

class CameraSystem : public ISystem {
    constexpr virtual std::string get_name() { return "Camera System"; }

    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;

    float base_speed = 4.0f;
    float sprint_mult = 3.0f;

    const std::string SPRINT = "Camera Sprint";
    const std::string FORWARD = "Camera Move Forward";
    const std::string BACKWARD = "Camera Move Backward";
    const std::string RIGHT = "Camera Move Right";
    const std::string LEFT = "Camera Move Left";
    const std::string UP = "Camera Move Up";
    const std::string DOWN = "Camera Move Down";
};
}  // namespace tmt