#pragma once

namespace tmt {
struct MotionMathComponent {
	MotionMathComponent() = default;

	float frequency = 1.f;
	float damping = 1.f;
	float initial_response = 0.f;
};
};  // namespace tmt