#pragma once
#include <ImReflect.hpp>
#include "engine/core/components/button.hpp"
#include "extern/magic_enum/magic_enum.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Button& button, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::Button>();
    auto& type_response = response.get<tmt::Button>();

    for (size_t i = 0; i < button.colors.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        const auto enum_value = static_cast<tmt::ButtonState>(i);
        const auto enum_label = std::string(magic_enum::enum_name(enum_value));
        ImReflect::Input(enum_label.c_str(), button.colors[i], type_settings, type_response);
        ImGui::PopID();
    }
}
