#pragma once
#include "ecs.hpp"

namespace tmt {

enum class ActionStatus { INACTIVE, RUNNING, SUCCESS, FAILED };

struct ActionState {
    ActionStatus status = ActionStatus::INACTIVE;
    float elapsed_time = 0.f;
};

}  // namespace tmt
