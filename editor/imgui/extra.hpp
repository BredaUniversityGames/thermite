#include <imgui.h>

inline void tooltip(const char* text) {
    if (!ImGui::BeginItemTooltip()) return;

    ImGui::Text("%s", text);

    ImGui::EndTooltip();
}