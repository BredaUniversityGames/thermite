#pragma once

#include <string>
#include <unordered_map>

#include "engine/core/reflection.hpp"

namespace tmt {

struct GoapActionEditorData {
    float cost = -1.f;  // -1 = use default
    std::unordered_map<std::string, bool> preconditions;
    std::unordered_map<std::string, bool> effects;
};

}  // namespace tmt

TMT_OBJECT(tmt::GoapActionEditorData, (cost, preconditions, effects));
