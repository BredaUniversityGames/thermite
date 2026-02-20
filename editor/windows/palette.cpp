#include "palette.hpp"

#include "editor.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/shared/colorspace.hpp"

#include <glm/gtc/packing.hpp>

#include "imgui_internal.h"
#include "engine/core/logger.hpp"

namespace ImGui {

// Copied from imgui_widgets.cpp.
void ColorEditRestoreH(const float* col, float* H) {
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0))) return;
    *H = g.ColorEditSavedHue;
}

// Copied from imgui_widgets.cpp.
// ColorEdit supports RGB and HSV inputs. In case of RGB input resulting color may have undefined hue and/or saturation.
// Since widget displays both RGB and HSV values we must preserve hue and saturation to prevent these values resetting.
static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V) {
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0))) return;

    // When S == 0, H is undefined.
    // When H == 1 it wraps around to 0.
    if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1)) *H = g.ColorEditSavedHue;

    // When V == 0, S is undefined.
    if (*V == 0.0f) *S = g.ColorEditSavedSat;
}

}  // namespace ImGui

namespace {

bool custom_color_wheel(const char* label, glm::vec3& color) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImDrawList* draw_list = window->DrawList;
    ImGuiStyle& style = g.Style;
    ImGuiIO& io = g.IO;

    const float width = ImGui::CalcItemWidth();
    const bool is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & ImGuiItemFlags_ReadOnly) != 0;
    g.NextItemData.ClearFlags();

    ImGui::PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id) g.ColorEditCurrentID = window->IDStack.back();

    ImGui::BeginGroup();

    // Setup.
    const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
    const ImU32 black = IM_COL32(0, 0, 0, style_alpha8);
    const ImU32 white = IM_COL32(255, 255, 255, style_alpha8);

    const ImVec2 picker_pos = window->DC.CursorPos;

    const float bars_width = ImGui::GetFrameHeight();                                                               // Arbitrary smallish width of Hue/Alpha picking bars.
    const float hs_picker_size = ImMax(bars_width * 1.0f, width - 1.0f * (bars_width + style.ItemInnerSpacing.x));  // Hue/Saturation/ picking circle.
    const float bar_pos_x = picker_pos.x + hs_picker_size + style.ItemInnerSpacing.x;
    const float handle_half_height = bars_width * 0.25f;

    const float wheel_thickness = hs_picker_size * 0.08f;
    const float wheel_radius = hs_picker_size * 0.5f;
    const ImVec2 wheel_center(picker_pos.x + hs_picker_size * 0.5f, picker_pos.y + hs_picker_size * 0.5f);

    bool hue_saturation_changed = false;
    bool value_changed = false;

    const glm::vec3 backup_initial_color = color;
    glm::vec3 nonlinear_srgb = tmt::cs::delinearize(tmt::cs::acescg_to_r709(color));

    float hue;
    float saturation;
    float value;
    ImGui::ColorConvertRGBtoHSV(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, hue, saturation, value);
    value = glm::clamp(value, 0.0f, 1.0f);
    ImGui::ColorEditRestoreHS(&nonlinear_srgb.x, &hue, &saturation, &value);

    // Handle hue/saturation circle behaviour.
    ImGui::InvisibleButton("HueSaturation", ImVec2(hs_picker_size, hs_picker_size));
    if (ImGui::IsItemActive() && !is_readonly) {
        const ImVec2 mouse_circle_pos = (io.MousePos - wheel_center) / wheel_radius;
        hue = glm::mod(glm::atan(mouse_circle_pos.y, mouse_circle_pos.x) / glm::tau<float>() + 1.0f, 1.0f);
        saturation = glm::min(1.0f, glm::sqrt(ImLengthSqr(mouse_circle_pos)));

        hue_saturation_changed = true;
    }

    // Handle value bar behaviour.
    ImGui::SetCursorScreenPos(ImVec2(bar_pos_x, picker_pos.y));
    ImGui::InvisibleButton("Value", ImVec2(bars_width, hs_picker_size));
    if (ImGui::IsItemActive() && !is_readonly) {
        value = 1.0f - glm::clamp((io.MousePos.y - (picker_pos.y + handle_half_height)) / (hs_picker_size - (handle_half_height * 2.0f) - 1.0f), 0.0f, 1.0f);
        value = glm::clamp(value, 0.0f, 1.0f);

        ImGui::ColorEditRestoreHS(&nonlinear_srgb.x, &hue, &saturation, &value);

        value_changed = true;
    }

    ImGui::EndGroup();

    if (value_changed) {
        g.ColorEditSavedHue = hue;
        g.ColorEditSavedSat = saturation;
        g.ColorEditSavedID = g.ColorEditCurrentID;
        g.ColorEditSavedColor = ImGui::ColorConvertFloat4ToU32(ImVec4(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, 0.0f));

        ImGui::ColorConvertHSVtoRGB(hue, saturation, value, nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b);
        color = tmt::cs::r709_to_acescg(tmt::cs::linearize(nonlinear_srgb));

        // ImGui::ColorConvertRGBtoHSV(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, hue, saturation, value);
        // ImGui::ColorEditRestoreHS(&nonlinear_srgb.x, &hue, &saturation, &value);
    }

    // Draw the color wheel in segments.
    const int wheel_segment_count = draw_list->_CalcCircleAutoSegmentCount(wheel_radius);
    for (int i = 0; i < wheel_segment_count; i++) {
        const float segment1 = static_cast<float>(i) / static_cast<float>(wheel_segment_count - 1);
        const float x1 = glm::cos(glm::tau<float>() * segment1) * wheel_radius;
        const float y1 = glm::sin(glm::tau<float>() * segment1) * wheel_radius;

        const float segment2 = static_cast<float>((i + 1) % wheel_segment_count) / static_cast<float>(wheel_segment_count - 1);
        const float x2 = glm::cos(glm::tau<float>() * segment2) * wheel_radius;
        const float y2 = glm::sin(glm::tau<float>() * segment2) * wheel_radius;

        ImVec4 rgb1(0.0f, 0.0f, 0.0f, 1.0f);
        ImGui::ColorConvertHSVtoRGB(segment1, 1.0f, value, rgb1.x, rgb1.y, rgb1.z);
        const ImU32 color1 = ImGui::ColorConvertFloat4ToU32(rgb1);

        ImVec4 rgb2(0.0f, 0.0f, 0.0f, 1.0f);
        ImGui::ColorConvertHSVtoRGB(segment2, 1.0f, value, rgb2.x, rgb2.y, rgb2.z);
        const ImU32 color2 = ImGui::ColorConvertFloat4ToU32(rgb2);

        const ImU32 no_saturation_color = ImGui::ColorConvertFloat4ToU32(ImVec4(value, value, value, 1.0f));

        const ImVec2 uv_white = ImGui::GetFontTexUvWhitePixel();
        draw_list->PrimReserve(3, 3);
        draw_list->PrimVtx(wheel_center + ImVec2(x2, y2), uv_white, color2);
        draw_list->PrimVtx(wheel_center, uv_white, no_saturation_color);
        draw_list->PrimVtx(wheel_center + ImVec2(x1, y1), uv_white, color1);
    }

    ImVec2 hs_cursor_pos(glm::cos(hue * glm::tau<float>()) * saturation * wheel_radius, glm::sin(hue * glm::tau<float>()) * saturation * wheel_radius);
    hs_cursor_pos += wheel_center;

    // Draw cursor/preview circle.
    const float hs_cursor_radius = (hue_saturation_changed) ? wheel_thickness * 0.55f : wheel_thickness * 0.40f;
    const int hs_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(hs_cursor_radius);  // Lock segment count so the +1 one matches others.
    draw_list->AddCircleFilled(hs_cursor_pos, hs_cursor_radius, ImGui::ColorConvertFloat4ToU32(ImVec4(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, 1.0f)), hs_cursor_segments);
    draw_list->AddCircle(hs_cursor_pos, hs_cursor_radius + 1, black, hs_cursor_segments);
    draw_list->AddCircle(hs_cursor_pos, hs_cursor_radius, white, hs_cursor_segments);

    // Draw value bar.
    draw_list->AddRectFilledMultiColor(ImVec2(bar_pos_x, picker_pos.y), ImVec2(bar_pos_x + bars_width, picker_pos.y + hs_picker_size), white, white, black, black);

    const float value_selection_y = picker_pos.y + handle_half_height + (hs_picker_size - (handle_half_height * 2.0f) - 1.0f) * (1 - value);
    const ImVec2 selection_min(bar_pos_x, value_selection_y - handle_half_height);
    const ImVec2 selection_max(bar_pos_x + bars_width, value_selection_y + handle_half_height);

    // Draw value bar handle.
    draw_list->AddRectFilled(selection_min, selection_max, white);
    draw_list->AddRect(selection_min, selection_max, black);

    bool changed = hue_saturation_changed || value_changed;
    if (changed && color == backup_initial_color) changed = false;
    if (changed && g.LastItemData.ID != 0)  // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        ImGui::MarkItemEdited(g.LastItemData.ID);

    if (set_current_color_edit_id) g.ColorEditCurrentID = 0;
    ImGui::PopID();

    if (changed) tmt::Log::info("hsv: {}, {}, {}", hue, saturation, value);

    return changed;
}

}  // namespace

namespace tmt {

void Palette::before_begin() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void Palette::end_display() {
    ImGui::PopStyleVar();
}

void Palette::display() {
    const auto selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
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

    const ImVec2 frame_size = ImGui::GetContentRegionAvail() - Config::OUTSET * 2.0f;
    const ImVec2 cursor = ImGui::GetCursorScreenPos() + Config::OUTSET * 2.0f;

    /* Draw palette grid */
    const float entry_width = frame_size.x / Config::GRID_SIZE.x;
    const float entry_height = frame_size.y / Config::GRID_SIZE.y;
    for (uint32_t y = 0u; y < Config::GRID_SIZE.y; ++y) {
        for (uint32_t x = 0u; x < Config::GRID_SIZE.x; ++x) {
            const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height);
            const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height);

            const uint32_t i = x + y * 8u;
            const Material& material = resource->blas->palette.entries[i];
            const glm::vec3 linear_srgb = cs::acescg_to_r709(material.albedo.unpack());
            glm::vec3 nonlinear_srgb = cs::delinearize(linear_srgb);

            const ImVec4 color = ImVec4(nonlinear_srgb.r, nonlinear_srgb.g, nonlinear_srgb.b, 1.0f);
            draw_list->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color));
            draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 3.0f);

            ImGui::SetCursorScreenPos(min);
            const std::string button_id = "##palette_entry_" + std::to_string(i);
            if (ImGui::InvisibleButton(button_id.c_str(), ImVec2(entry_width, entry_height))) {
                selected_material_index = static_cast<MaterialIndex>(i);
            }
        }
    }

    { /* Draw selected outline */
        const uint32_t x = selected_material_index & 0b111u;
        const uint32_t y = selected_material_index >> 3;

        const ImVec2 min = cursor + ImVec2(x * entry_width, y * entry_height) - Config::OUTSET;
        const ImVec2 max = cursor + ImVec2((x + 1u) * entry_width, (y + 1u) * entry_height) + Config::OUTSET;

        draw_list->AddRect(min, max, ImColor(0xFF000000u), 0.0f, 0, 6.0f);
        draw_list->AddRect(min, max, ImColor(0xFFFFFFFFu), 0.0f, 0, 3.0f);
    }
}

void Palette::display_material_editor() const {
    /* Doesn't actually work yet but for the idea */
    const auto selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();
    if (selected_entity == entt::null) {
        ImGui::TextWrapped("No voxel model selected.");
        return;
    }

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr) return;

    Material& material = renderer->resource->blas->palette.entries[selected_material_index];

    ImGui::BeginGroup();

    glm::vec3 albedo = material.albedo.unpack();
    // static glm::vec3 albedo {};
    if (custom_color_wheel("Albedo", albedo)) material.albedo = Rgb10(albedo);

    // glm::vec3 nonlinear_srgb = cs::delinearize(cs::acescg_to_r709(material.albedo.unpack()));
    // if (ImGui::ColorPicker3("Albedo Color", &nonlinear_srgb.r, ImGuiColorEditFlags_NoOptions)) material.albedo = Rgb10(cs::r709_to_acescg(cs::linearize(nonlinear_srgb)));

    float roughness = static_cast<float>(material.roughness) / 255.0f;
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.roughness = static_cast<uint8_t>(roughness * 255.0f);

    float metallic = static_cast<float>(material.metallic) / 255.0f;
    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.metallic = static_cast<uint8_t>(metallic * 255.0f);

    float ior = glm::unpackHalf1x16(material.ior);
    if (ImGui::SliderFloat("IOR", &ior, 1.0f, 3.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.ior = glm::packHalf1x16(ior);

    float transmission = static_cast<float>(material.transmission) / 255.0f;
    if (ImGui::SliderFloat("Transmission", &transmission, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_ClampOnInput)) material.transmission = static_cast<uint8_t>(transmission * 255.0f);

    float emission = glm::unpackHalf1x16(material.emission);
    if (ImGui::DragFloat("Emission", &emission, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_ClampOnInput)) material.emission = glm::packHalf1x16(emission);

    ImGui::EndGroup();

    if (ImGui::IsItemEdited()) renderer->resource->set_dirty();
}

}  // namespace tmt