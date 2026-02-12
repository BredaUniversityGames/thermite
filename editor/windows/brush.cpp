#include "brush.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>

#include "editor/imgui/types/all.hpp"
#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"

#include <imgui.h>
#include <ImReflect_entry.hpp>

namespace tmt {

const std::unordered_map<Brush::Tool, const char*> Brush::MODE_ICONS {
    { Tool::MULTI_TOOL, ICON_MS_OPEN_WITH },
    { Tool::COLOR_PICKER, ICON_MS_COLORIZE },
    { Tool::SINGLE, ICON_MS_DEPLOYED_CODE },
    { Tool::BOX, ICON_MS_GRID_ON },
};

void Brush::display() {
    for (const auto& [mode, icon] : MODE_ICONS) {
        // Keep everything on the same line (skip ImGui::SameLine on the first element).
        if (mode != MODE_ICONS.begin()->first) ImGui::SameLine();

        ImGui::BeginDisabled(active_tool == mode);

        if (ImGui::Button(icon)) active_tool = mode;

        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Settings");

    switch (active_tool) {
        case Tool::MULTI_TOOL: {
            const Entity selected_node = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
            if (!engine.ecs.valid(selected_node)) break;

            const char* space_button_icon = (editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR);
            if (ImGui::Button(space_button_icon)) editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);

            ImGui::SameLine();

            // This button toggles between move and rotate, since scaling in the voxel editor isn't helpful.
            const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
            if (ImGui::Button(mode_button_icon)) editor.gizmo.operation = (editor.gizmo.operation + 1) % Gizmo::GIZMO_OP_COUNT;

            ImGui::NewLine();

            Transform& transform = engine.ecs.get_component<Transform>(selected_node);
            ImReflect::Input("", transform);

            break;
        }

        case Tool::COLOR_PICKER:
            break;

        case Tool::SINGLE:
        case Tool::BOX:
            const float width = ImGui::GetContentRegionAvail().x;
            const float button_width = (width - ImGui::GetStyle().FramePadding.x * 2.0f) / 3.0f;

            ImGui::BeginDisabled(active_mode == Mode::ATTACH);
            if (ImGui::Button(ICON_MS_ADD, ImVec2 { button_width, 0.0f })) active_mode = Mode::ATTACH;
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetItemRectMax().x);

            ImGui::BeginDisabled(active_mode == Mode::REMOVE);
            if (ImGui::Button(ICON_MS_REMOVE, ImVec2 { button_width, 0.0f })) active_mode = Mode::REMOVE;
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetItemRectMax().x);

            ImGui::BeginDisabled(active_mode == Mode::PAINT);
            if (ImGui::Button(ICON_MS_BRUSH, ImVec2 { button_width, 0.0f })) active_mode = Mode::PAINT;
            ImGui::EndDisabled();

            break;
    }
}

}  // namespace tmt