#include "material_editor.hpp"

#include "editor.hpp"
#define IM_CUSTOM_COLORSPACE_MATRIX { 0.613097f, 0.070194f, 0.020616f, 0.339523f, 0.916354f, 0.109570f, 0.047380f, 0.013452f, 0.869815f }
#include "editor/shared/imgui_color_wheel.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/palette.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include <glm/gtc/packing.hpp>
#include <imgui.h>

namespace {

template <typename Type>
void modify_all_palettes(const std::vector<tmt::MaterialIndex>& material_indices, Type tmt::Material::* member, const Type value) {
    const entt::basic_group group = tmt::engine.ecs.group<tmt::VoxelRenderer>(entt::get<tmt::Transform>);

    // Loop over every renderer in the scene and modify their palettes.
    for (const auto&& [entity, renderer, transform] : group.each()) {
        for (const tmt::MaterialIndex material_index : material_indices) {
            tmt::Material& material = renderer.resource->blas->palette.entries[material_index];

            // Set the value though member pointer (means I only had to create 1 function for all material members).
            material.*member = value;
        }

        // Make sure to set all of them dirty.
        renderer.resource->set_dirty();
    }
}

template <typename Type>
void set_material_property(tmt::MaterialPalette* material_palette, const std::vector<tmt::MaterialIndex>& material_indices, Type tmt::Material::* member, const Type value) {
    // Material palette might be nullptr, meaning that all palettes must be modified.
    if (material_palette == nullptr) {
        modify_all_palettes(material_indices, member, value);
        return;
    }

    for (const tmt::MaterialIndex material_index : material_indices) {
        tmt::Material& material = material_palette->entries[material_index];

        // Set the value though member pointer (means I only had to create 1 function for all material members).
        material.*member = value;
    }
}

}  // namespace

namespace tmt {

void MaterialEditor::on_inspect() {
    ImGui::Checkbox("Modify all palettes", &edit_all_palettes);

    ImGui::Separator();

    const NodeHierarchy& hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();

    const auto selected_entity = hierarchy.get_first_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    Palette& palette = editor.systems[Editor::Mode::VOXEL].get<Palette>();
    const auto& all_material_indices = palette.get_selected_material_indices();
    const MaterialIndex material_index = all_material_indices.front();
    Material& material = renderer->resource->blas->palette.entries[material_index];

    ImGui::BeginGroup();

    static glm::vec3 albedo {};
    static glm::vec3 edge_tint {};
    if (palette.update_material_editor) {
        albedo = material.albedo.unpack();
        edge_tint = material.edge_tint.unpack();
        palette.update_material_editor = false;
    }
    // We make sure to clamp the Rgb10 color to the range 0.0f-1.0f
    if (ImGui::ColorWheel3("Albedo", &albedo.x, ImGuiColorWheelFlags_CustomColorSpace)) {
        const Rgb10 value { glm::clamp(albedo, glm::zero<glm::vec3>(), glm::one<glm::vec3>()) };
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::albedo, value);
    }

    // We make sure to clamp the Rgb10 color to the range 0.0f-1.0f
    if (ImGui::ColorWheel3("Edge Tint", &edge_tint.x, ImGuiColorWheelFlags_CustomColorSpace)) {
        const Rgb10 value { glm::clamp(edge_tint, glm::zero<glm::vec3>(), glm::one<glm::vec3>()) };
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::edge_tint, value);
    }

    float roughness = static_cast<float>(material.roughness) / 255.0f;
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        const uint8_t value = static_cast<uint8_t>(roughness * 255.0f);
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::roughness, value);
    }

    float metallic = static_cast<float>(material.metallic) / 255.0f;
    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        const uint8_t value = static_cast<uint8_t>(metallic * 255.0f);
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::metallic, value);
    }

    float ior = glm::unpackHalf1x16(material.ior);
    if (ImGui::SliderFloat("IOR", &ior, 1.0f, 3.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        const uint16_t value = glm::packHalf1x16(ior);
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::ior, value);
    }

    float transmission = static_cast<float>(material.transmission) / 255.0f;
    if (ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        const uint8_t value = static_cast<uint8_t>(transmission * 255.0f);
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::transmission, value);
    }

    float emission = glm::unpackHalf1x16(material.emission);
    if (ImGui::DragFloat("Emission", &emission, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        const uint16_t value = glm::packHalf1x16(emission);
        set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::emission, value);
    }

    if (ImGui::BeginCombo("Type", magic_enum::enum_name(material.type).data())) {
        for (const auto& [value, name] : magic_enum::enum_entries<Material::Type>()) {
            if (!ImGui::Selectable(name.data(), material.type == value)) continue;

            set_material_property((edit_all_palettes ? nullptr : &renderer->resource->blas->palette), all_material_indices, &Material::type, value);
        }

        ImGui::EndCombo();
    }

    ImGui::EndGroup();

    if (ImGui::IsItemEdited()) renderer->resource->set_dirty();
}

}  // namespace tmt