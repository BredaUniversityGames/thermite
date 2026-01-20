#include "debug_lines.hpp"
#include "debug_lines.hpp"

#include <imgui.h>

#include "editor/editor.hpp"
#include "engine/events/debug.hpp"

namespace tmt {

void DebugLines::display() {
    // Todo move this to menu bar at some point
    if (ImGui::BeginMenu("Debug Lines")) {
        auto& enabled_debug_renderers = editor.save_data.enabled_debug_renderers;
        for (const auto& listener : OnDrawLines::get_listeners()) {
            const auto& name = listener->get_name();

            if (enabled_debug_renderers.contains(name) == false) {
                enabled_debug_renderers[name] = listener->default_enabled();
            }
            bool& enabled = enabled_debug_renderers[name];

            ImGui::MenuItem(name.data(), nullptr, &enabled);
        }
        ImGui::EndMenu();
    }
}

void DebugLines::on_editor_update(const tmt::FrameData&) {
    auto& enabled_debug_renderers = editor.save_data.enabled_debug_renderers;
    for (const auto& listener : OnDrawLines::get_listeners()) {
        const auto& name = listener->get_name();
        if (enabled_debug_renderers.contains(name) && enabled_debug_renderers[name]) {
            listener->on_draw_lines();
        }
    }
}

void DebugLines::on_draw_lines() { printf("Debug Lines drawn\n"); }

}  // namespace tmt