#include "pop_up.hpp"

#include "engine/core/logger.hpp"

#include <imgui.h>
#include "editor/font/icon_lookups.hpp"
#include "editor/imgui/tools/center.hpp"

namespace tmt {

static const ImVec4 severity_colors[] = {
    ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // INFO
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // WARNING
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // ERROR
    ImVec4(1.0f, 0.5f, 0.0f, 1.0f)   // CRITICAL
};

static const char* severity_icons[] = {
    ICON_MS_INFO,      // INFO
    ICON_MS_WARNING,   // WARNING
    ICON_MS_ERROR,     // ERROR
    ICON_MS_DANGEROUS  // CRITICAL
};

void PopUpManager::update() {
    if (queue.empty()) return;

    const PopUp& first_pop_up = queue.front();

    ImGuiIO& io = ImGui::GetIO();
    /* Center pop-up */
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::OpenPopup(first_pop_up.title_.c_str());

    auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    bool open = true;
    if (ImGui::BeginPopupModal(first_pop_up.title_.c_str(), &open, flags)) {
        const auto color = severity_colors[static_cast<int>(first_pop_up.severity_)];
        const auto icon = severity_icons[static_cast<int>(first_pop_up.severity_)];

        ImGui::PushFont(NULL, ImGui::GetStyle().FontSizeBase * 2.0f);
        ImGui::TextColored(color, "%s", icon);
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::Text("%s", first_pop_up.message_.c_str());

        {
            /* Code to center buttons, copied from Claudia */
            float total_buttons_width = 0.0f;
            const float button_spacing = ImGui::GetStyle().ItemSpacing.x;

            for (const auto& button : first_pop_up.buttons) {
                total_buttons_width += ImGui::CalcTextSize(button.label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            }

            if (first_pop_up.buttons.size() > 1) total_buttons_width += button_spacing * (first_pop_up.buttons.size() - 1);

            float center_offset = (ImGui::GetContentRegionAvail().x - total_buttons_width) * 0.5f;
            if (center_offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + center_offset);

            for (const auto& button : first_pop_up.buttons) {
                if (ImGui::Button(button.label.c_str())) {
                    if (button.callback) button.callback();
                    if (button.closes_pop_up) queue.erase(queue.begin());
                }
                ImGui::SameLine();
            }
        }

        ImGui::EndPopup();
    }

    if (open == false) {
        ImGui::CloseCurrentPopup();
        queue.erase(queue.begin());
    }
}

void NotificationManager::update() {}

}  // namespace tmt