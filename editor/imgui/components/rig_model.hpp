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
        ImReflect::Input("", value.data);

        if (ImGui::Button("Load Rig")) {
            const auto entity = tmt::engine.ecs.get_entity(value);
            value.init(value.data.file_location, entity);
        }

    } else if (!value.vox_is_loaded) {
        ImGui::Text("Voxel not loaded");
        ImReflect::Input("", value.vox_path);

        if (ImGui::Button("Attach Voxels")) {
            value.attach_voxel_objects();
        }
    }

    if (!value.data) return;

    ImGui::Text("current animation: ");
    ImGui::Indent();
    ImGui::Text("%s", value.current_animation_playing().c_str());
    ImGui::Unindent();

    for (const auto& [location, animation_name] : value.data->animation_files) {
        ImGui::Text("Animation: %s", animation_name.c_str());
        if (ImGui::Button(("Play " + animation_name).c_str())) {
            value.play_animation(animation_name, 1.0f, true);
        }
    }

    if (value.rig_is_loaded) {
        if (ImGui::Button("Reload Rig")) value.data->reload();

        ImGui::NewLine();

        auto& animation_files = value.data->animation_files;

        auto iterator = animation_files.end();
        for (auto&& [file, name] : animation_files) {
            ImGui::PushID(file.get_relative_path().c_str());

            if (ImGui::SmallButton(ICON_MS_DELETE)) iterator = animation_files.find(file);
            ImGui::SameLine();
            ImGui::Text("%s", name.c_str());

            ImGui::PopID();
        }
        if (iterator != animation_files.end()) {
            animation_files.erase(iterator);
            value.data->reload();
        }

        if (ImGui::Button(ICON_MS_ADD " Add animation...")) ImGui::OpenPopup("Add Animation");

        static tmt::ResourceRef<tmt::RigData> animation_file;
        if (ImGui::BeginPopup("Add Animation")) {
            ImReflect::Input("Animation File", animation_file);

            if (animation_file != nullptr && animation_file->is_loaded()) {
                const auto& bones = animation_file->bones;
                if (!bones.empty() && !bones.front().animations.empty()) {
                    animation_files.emplace(animation_file.file_location, bones.front().animations.begin()->first);
                    value.data->reload();
                }

                animation_file = {};  // Clear the resource.
            }

            ImGui::EndPopup();
        }

        if (animation_file != nullptr && !ImGui::IsPopupOpen("Add Animation")) animation_file = {};
    }
}