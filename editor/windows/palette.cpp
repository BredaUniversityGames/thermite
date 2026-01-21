#include "palette.hpp"

#include "editor.hpp"
#include "imgui_internal.h"
#include "editor/windows/node_hierarchy.hpp"

namespace tmt {

void Palette::display() {
    const auto selected_node = editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().selected_node;
    if (selected_node == nullptr || selected_node->tree == nullptr) return;

    const float frame_height = ImGui::GetFrameHeight();
    for (int32_t i = 0; i < 256; i++) {
        const Material& material = selected_node->tree->palette[i - 1];

        float available_width = ImGui::GetContentRegionAvail().x;

        const ImVec4 color {material.albedo_r, material.albedo_g, material.albedo_b, 1.0f};
        if (ImGui::ColorButton(std::to_string(i).c_str(), color, ImGuiColorEditFlags_None)) selected_material_index = static_cast<MaterialIndex>(i);

        available_width -= ImGui::GetItemRectSize().x;
        if (available_width > frame_height) ImGui::SameLine();

        if (selected_material_index == i) ImGui::RenderDragDropTargetRectEx(ImGui::GetWindowDrawList(), GImGui->LastItemData.Rect);
    }
}

}  // namespace tmt