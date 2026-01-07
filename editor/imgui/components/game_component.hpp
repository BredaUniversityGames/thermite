#pragma once
#include <ImReflect.hpp>

namespace tmt {
/* Forward declare */
class IGameComponent;
class ComponentCollection;
}

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::IGameComponent& value, ImSettings& settings, ImResponse& response);

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ComponentCollection& value, ImSettings& settings, ImResponse& response);
