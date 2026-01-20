#pragma once

#include <ImReflect.hpp>

#include "engine/core/components/light.hpp"
#include "editor/font/icon_lookups.hpp"

inline void help_tooltip(const char* desc) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s", desc);
}

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Light& value, ImSettings&, ImResponse&) {
    auto tab_flags = [&](tmt::LightType t) {
        return (value.type == t) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
    };

    /* List of light type labels */
    const char* LIGHT_TYPE_LABELS[] {"Sphere Light", "Sun Light", "Spot Light", "Tube Light"};
    const uint32_t selected = (uint32_t)magic_enum::enum_index<tmt::LightType>(value.type).value_or(0u);

    /* Light type selection */
    if (ImGui::BeginCombo("Light Type", LIGHT_TYPE_LABELS[selected])) {
        constexpr uint32_t COUNT = sizeof(LIGHT_TYPE_LABELS) / sizeof(char*);
        for (uint32_t i = 0u; i < COUNT; ++i) {
            const bool is_selected = (selected == i);
            if (ImGui::Selectable(LIGHT_TYPE_LABELS[i], is_selected))
                value.type = magic_enum::enum_cast<tmt::LightType>(i).value_or(tmt::LightType::SPHERE_LIGHT);
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    /* Light type specific parameters */
    switch (value.type) {
        /* Sphere area light */
        case tmt::LightType::SPHERE_LIGHT: {
            tmt::SphereLight light = std::get<tmt::SphereLight>(value.light);

            ImGui::DragFloat("Source Radius", &light.source_radius, 0.025f, 0.0f, light.attenuation_radius);
            help_tooltip("Radius of the sphere light source. (larger means softer shadows)");
            ImGui::DragFloat("Attenuation Radius", &light.attenuation_radius, 0.1f, light.source_radius, 100.0f);
            help_tooltip("Radius at which the light influence will be zero.");
            
            ImGui::DragFloat("Luminous Flux", &light.luminous_flux, 10.0f, 0.0f, 10000.0f);
            help_tooltip("Luminous flux of the sphere light. (in lumen, default: 100lm)");

            value.light = light;
            break;
        }
    }
    
    ImGui::Separator();

    /* Shared parameters */
    ImGui::ColorEdit3("Color", &value.color.x);
    help_tooltip("Color of the light emitted. (multiplied together with temperature)");
    ImGui::DragFloat("Temperature", &value.temperature);
    help_tooltip("Temperature of the light emitted. (in kelvin, default: 6500k)");
}
