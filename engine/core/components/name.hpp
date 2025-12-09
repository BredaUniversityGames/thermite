#pragma once
#include <string>
#include <visit_struct/visit_struct.hpp>
#include <ImReflect_macro.hpp>

#include "engine/tools/fmt/helper.hpp"

namespace tmt {
struct Name {
    std::string name {"Unnamed Entity"};
};
}  // namespace tmt

FMT_LOGGING(tmt::Name, "name: \"{}\"", obj.name);
IMGUI_REFLECT(tmt::Name, name);