#include "motion_math.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/motion_math/motion_math_system.hpp"
#include <imgui.h>

void tmt::MotionMathPreview::display() {
    auto& motion_math_sys = tmt::engine.ecs.systems.get<tmt::MotionMathSystem>();

    ImGui::InputFloat("Frequency", &frequency);
    ImGui::InputFloat("Damping", &damping);
    ImGui::InputFloat("Initial response", &initial_response);
}
