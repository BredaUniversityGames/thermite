#pragma once
#include "ecs.hpp"

namespace tmt {

struct fact_pair;

struct ActionComponent {
    Entity agent;  // Which agent owns this action entity

    // Preconditions required for this action
    std::vector<fact_pair> preconditions;

    // Effects applied by this action
    std::vector<fact_pair> effects;
};

}  // namespace tmt
