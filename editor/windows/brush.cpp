#include "brush.hpp"

#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"

#include <engine/engine.hpp>
#include <engine/core/ecs.hpp>

#include <imgui.h>
#include <ImReflect_entry.hpp>

namespace tmt {

const std::unordered_map<Brush::Mode, const char*> Brush::mode_icons {
    {Mode::MULTI_TOOL, ICON_MS_OPEN_WITH}, {Mode::PAINT, ICON_MS_BRUSH},        {Mode::COLOR_PICKER, ICON_MS_COLORIZE},
    {Mode::ADD, ICON_MS_ADD_BOX},          {Mode::REMOVE, ICON_MS_DESTRUCTION}, {Mode::SHAPE, ICON_MS_SHAPE_LINE},
};

void Brush::display() {
    for (const auto& [mode, icon] : mode_icons) {
        // Keep everything on the same line (skip ImGui::SameLine on the first element).
        if (mode != mode_icons.begin()->first) ImGui::SameLine();

        ImGui::BeginDisabled(active_mode == mode);

        if (ImGui::Button(icon)) active_mode = mode;

        ImGui::EndDisabled();
    }

    ImGui::SeparatorText("Settings");

    switch (active_mode) {
        case Mode::MULTI_TOOL: {
            const Entity selected_node = editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().get_selected_entity();
            if (!engine.ecs.valid(selected_node)) break;

            Transform& transform = engine.ecs.get_component<Transform>(selected_node);
            // ImReflect::Input("", transform);
            break;
        }

        case Mode::PAINT: {
            break;
        }

        case Mode::COLOR_PICKER: {
            break;
        }

        case Mode::ADD: {
            break;
        }

        case Mode::REMOVE: {
            break;
        }

        case Mode::SHAPE: {
            break;
        }
    }
}

}  // namespace tmt