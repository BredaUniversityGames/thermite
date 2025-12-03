#pragma once

#include <string>

#include "core/system.hpp"

namespace tmt {

const std::string M_SPRINT = "Camera Sprint";
const std::string M_FORWARD = "Camera Move Forward";
const std::string M_BACKWARD = "Camera Move Backward";
const std::string M_RIGHT = "Camera Move Right";
const std::string M_LEFT = "Camera Move Left";
const std::string M_UP = "Camera Move Up";
const std::string M_DOWN = "Camera Move Down";

class CameraSystem : public ISystem {
    
    constexpr virtual std::string get_name() { return "Camera System"; }

    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;
};
} // namespace tmt