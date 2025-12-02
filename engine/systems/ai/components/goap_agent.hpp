#pragma once
#include "ecs.hpp"
#include "world_state.hpp"
#include "goal_component.hpp"
#include "action_state.hpp"
#include <vector>

namespace tmt {

struct GOAPAgent {
    std::vector<entt::entity> available_actions; // List of available actions specific to this agent.

    std::vector<entt::entity> current_plan;
    size_t current_action_index = 0;

    bool needs_replan = true;
    bool has_goal = false;

    ActionState action_state;                   // Status of the current running action.
};


}  // namespace tmt
