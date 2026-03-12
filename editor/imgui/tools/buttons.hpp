#pragma once
#include <imgui.h>

namespace ImGui {

bool SquareButton(const char* label) {
    const auto available_size = ImGui::GetContentRegionAvail();
    const auto text_size = ImGui::CalcTextSize(label);
    const auto frame_padding = ImGui::GetStyle().FramePadding;
    const auto button_size = ImVec2(text_size.x + frame_padding.x * 2.0f, text_size.y + frame_padding.y * 2.0f);

}

}  // namespace ImGui