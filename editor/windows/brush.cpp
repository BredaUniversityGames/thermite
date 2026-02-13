#include "brush.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>
#include <engine/tools/serializer/all.hpp>

#include "editor.hpp"
#include "editor/imgui/types/all.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"

#include <imgui.h>
#include <ImReflect_entry.hpp>

namespace tmt {

namespace {

const std::vector<std::pair<Brush::Tool, const char*>> TOOL_ICONS {
    { Brush::Tool::MULTI_TOOL, ICON_MS_OPEN_WITH },
    { Brush::Tool::COLOR_PICKER, ICON_MS_COLORIZE },
    { Brush::Tool::SINGLE, ICON_MS_DEPLOYED_CODE },
    { Brush::Tool::BOX, ICON_MS_GRID_ON },
};

const std::vector<std::pair<Brush::Mode, const char*>> MODE_ICONS {
    { Brush::Mode::ATTACH, ICON_MS_ADD },
    { Brush::Mode::REMOVE, ICON_MS_REMOVE },
    { Brush::Mode::PAINT, ICON_MS_BRUSH },
};

}  // namespace

void Brush::display() {
    for (const auto [tool, icon] : TOOL_ICONS) {
        // Keep everything on the same line (skip ImGui::SameLine on the first element).
        if (tool != TOOL_ICONS.begin()->first) ImGui::SameLine();

        ImGui::BeginDisabled(state.tool == tool);

        if (ImGui::Button(icon)) state.tool = tool;

        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Settings");

    switch (state.tool) {
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
            ImResponse response = ImReflect::Input("", transform);

            // Handle undo/redo of the transform using the component diff.
            const auto transform_response = response.get<Transform>();
            static std::unique_ptr<ComponentDiff<Transform>> transform_diff;
            if (transform_response.is_activated()) {
                transform_diff = std::make_unique<ComponentDiff<Transform>>(selected_node);
                transform_diff->before();
            } else if (transform_response.is_deactivated_after_edit()) {
                transform_diff->after();
                transform_diff->commit();
            }
            break;
        }

        case Tool::COLOR_PICKER:
            break;

        case Tool::SINGLE:
        case Tool::BOX:
            const float width = ImGui::GetContentRegionAvail().x;
            const float button_width = (width - ImGui::GetStyle().FramePadding.x * 2.0f) / 3.0f;

            for (const auto [mode, icon] : MODE_ICONS) {
                // Keep everything on the same line (skip ImGui::SameLine on the first element).
                if (mode != MODE_ICONS.begin()->first) {
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetItemRectMax().x);
                }

                ImGui::BeginDisabled(state.mode == mode);
                if (ImGui::Button(icon, ImVec2 { button_width, 0.0f })) state.mode = mode;
                ImGui::EndDisabled();
            }

            break;
    }
}

}  // namespace tmt