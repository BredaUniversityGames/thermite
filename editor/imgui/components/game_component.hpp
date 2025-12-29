#pragma once
#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/core/game/game_component.hpp"

#include "engine/core/game/game_component_registry.hpp"
#include "engine/core/components/component_collection.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::IGameComponent& value, ImSettings& settings, ImResponse& response) {
    // call inspect
    value.inspect();
}

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ComponentCollection& value, ImSettings& settings, ImResponse& response) {
    for (auto& [type_id, component] : value.get_all_components()) {
        const auto& component_info = tmt::engine.component_registry.get_component_info(type_id);
        ImReflect::Input(component_info.name.c_str(), component, settings, response);
    }

    const auto& registered_components = tmt::engine.component_registry.get_registered_components();
    for (const auto& [type_id, component_info] : registered_components) {
        if (value.has_component(type_id)) continue;
     
        ImGui::PushID(component_info.name.c_str());
        if (ImGui::Button(("Add " + component_info.name).c_str())) {
            value.add_component(type_id);
        }
        ImGui::PopID();
    }
}
