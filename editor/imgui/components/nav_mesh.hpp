#pragma once
#include <ImReflect.hpp>

#include "engine/systems/ai/navigation/nav_mesh.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::NavMesh& value, ImSettings&, ImResponse&) {
    //load rig model if not loaded and make field for file location
    
    if(ImGui::Button("Average normals"))
    {
        value.average_neighbor_normals();
    }
}