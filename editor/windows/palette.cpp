#include "palette.hpp"

#include "editor.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <imgui_internal.h>

namespace tmt {

void display_material_editor() {
    ImGui::Text("Material Editor");
}

void Palette::display() {
    const auto selected_entity = editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().get_selected_entity();
    if (selected_entity == entt::null) return;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    display_palette(renderer->resource);

    if (ImGui::Begin(ICON_MS_EDIT " Material Editor")) {
        display_material_editor();
    }
    ImGui::End();
}

void Palette::display_palette(const ResourceRef<VoxelVolume>& resource) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImVec2 frame_size = ImGui::GetContentRegionAvail();
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    /* Draw palette grid */
    const float entry_width = frame_size.x / 8.0f;
    const float entry_height = frame_size.y / 32.0f;
    for (uint32_t y = 0u; y < 32u; ++y) {
        for (uint32_t x = 0u; x < 8u; ++x) {
            const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height);
            const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height);

            const uint32_t i = x + y * 8u;
            const Material& material = resource->blas->palette.entries[i];

            const ImVec4 color = ImVec4(material.albedo_r, material.albedo_g, material.albedo_b, 1.0f);
            draw_list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color));
            draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 3.0f);
        }
    }

    { /* Draw selected outline */
        const uint32_t x = selected_material_index & 0b111u;
        const uint32_t y = selected_material_index >> 3;

        const ImVec2 outset = ImVec2(2.0f, 2.0f);
        const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height) - outset;
        const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height) + outset;

        draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 6.0f);
        draw_list->AddRect(min, max, ImColor(0xFFFFFFFFu), 0.0f, 0, 3.0f);
    }
    
    if (!ImGui::IsMouseHoveringRect(cursor, cursor + frame_size)) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    const ImVec2 mouse_pos = ImGui::GetMousePos();
    const uint32_t x = (uint32_t)((mouse_pos.x - cursor.x) / frame_size.x * 8.0f);
    const uint32_t y = (uint32_t)((mouse_pos.y - cursor.y) / frame_size.y * 32.0f);
    selected_material_index = MaterialIndex(x + y * 8u);
}

}  // namespace tmt