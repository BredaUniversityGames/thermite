#pragma once
#include <ImReflect.hpp>

#include "engine/systems/animation/rig_model.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::RigModel& value, ImSettings&, ImResponse&) {
    // load rig model if not loaded and make field for file location
    if (!value.rig_is_loaded) {
        ImGui::Text("Rig not loaded");
        ImReflect::Input("Animation Resource", value.rig_path);
    }
    if (!value.rig_path.relative_path.empty() && !value.rig_is_loaded) {
        auto entity = tmt::engine.ecs.get_entity(value);
        value.init(entity);
    }

    if (value.rig_is_loaded) {
        if (!value.vox_is_loaded) {
            ImGui::Text("Vox not loaded");
            ImReflect::Input("Voxel Resource", value.vox_path);
        }
        if (!value.vox_path.relative_path.empty() && !value.vox_is_loaded) {
            value.attach_voxel_objects();
        }
    }

    if (!value.data) return;
    ImReflect::Input("Animation file", value.data->animation_files);
    if (ImGui::Button("Add Animation")) {
        value.data->reload();
    }

    for (const auto& [location, animation_name] : value.data->animation_files) {
        ImGui::Text("Animation: %s", animation_name.c_str());
        if (ImGui::Button(("Play " + animation_name).c_str())) {
            value.play_animation(animation_name, 1.0f, true);
        }
    }
}
