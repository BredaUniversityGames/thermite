#include "motion_math.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/motionmath/motion_math_system.hpp"
#include <imgui.h>

void tmt::MotionMathPreview::display() 
{ 
	auto& motion_math_sys = tmt::engine.ecs.systems.get<tmt::MotionMathSystem>();
	
	ImGui::InputFloat("Frequency", &motion_math_sys.temp_motion_settings.frequency);
    ImGui::InputFloat("Damping", &motion_math_sys.temp_motion_settings.damping);
    ImGui::InputFloat("Initial response", &motion_math_sys.temp_motion_settings.initial_response);
}
