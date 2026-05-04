#pragma once

#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/systems/physics/components/voxel_connection.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::VoxelConnection& connection, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::VoxelConnection>();
    auto& type_response = response.get<tmt::VoxelConnection>();

    //ImReflect::Input("Connections", connection.connections, type_settings, type_response);
    if (ImGui::Button("Add Connection")) {
        connection.connections.push_back(tmt::EntityLink {});
        type_response.changed();
    }
    for (tmt::EntityLink link : connection.connections) {
        ImReflect::Input("Connected Entity", link.connected_entity, type_settings, type_response);
        ImGui::Button("Update Connection");
    }

    //ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);

    //ImReflect::Detail::imgui_input_visit_field(name, connection.connections, settings, response);
    
    //JsonReflect::Detail::from_json_visitable(j, value, std::forward<Args>(args)...);
}