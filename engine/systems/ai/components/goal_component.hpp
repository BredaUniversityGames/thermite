#pragma once
#include "world_state.hpp"
#include <vector>

namespace tmt {

struct GoalComponent {
    std::vector<fact_pair> desired_state;
    int priority = 0;
};

}  // namespace tmt
