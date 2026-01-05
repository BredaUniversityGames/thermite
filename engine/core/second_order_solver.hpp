#pragma once
#include "glm/gtc/constants.hpp"

namespace tmt
{
	class SecondOrderSolver
	{
   public:

		template<typename T>
	    struct State
		{
			T previous_input, new_solved, current_velocity;
		};

	    template<typename T>
		static void Solve(State<T>& state, const T& target, float f, float z, float r, float delta_time)
		{
			const float k1 = z / (glm::pi<float>() * f);
            const float k2 = 1.f / ((2.f * glm::pi<float>() * f) * (2.f * glm::pi<float>() * f));
			const float k3 = (r * z) / (2.f * glm::pi<float>() * f);

			T estimated_velocity = (target - state.previous_input) / delta_time;
			state.previous_input = target;

			state.new_solved = state.new_solved + delta_time * state.current_velocity;
			state.current_velocity = state.current_velocity + delta_time * (target + k3*estimated_velocity - state.new_solved - k1*state.current_velocity) / k2;
		}
	};
}