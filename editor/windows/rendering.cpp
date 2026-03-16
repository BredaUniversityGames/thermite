#include "rendering.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/render_view.hpp"

#include "editor/overlays/lights.hpp"

namespace tmt {

void Rendering::on_editor_start() {}

void Rendering::on_editor_update(const tmt::FrameData&) {}

void Rendering::on_editor_end() {}

void Rendering::display() {
    ImGui::DragFloat("Sample Radius", &engine.renderer.render_view.sample_radius, 0.1f, 0.0f, 256.0f, "%.2f px");
    ImGui::DragFloat("Color Weight", &engine.renderer.render_view.color_weight, 0.01f, 0.0f, 256.0f, "%.4f");
    ImGui::DragFloat("Normal Weight", &engine.renderer.render_view.normal_weight, 0.01f, 0.0f, 256.0f, "%.4f");
    ImGui::DragFloat("Position Weight", &engine.renderer.render_view.pos_weight, 0.01f, 0.0f, 256.0f, "%.4f");
}

void Rendering::on_draw_lines() const {
    draw_lights_overlay();
}

}  // namespace tmt
