#include "ui_component.hpp"
#include <ImReflect.hpp>
#include "editor/imgui/types/glm.hpp"

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::UIComponent& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::UIComponent>();
    auto& type_response = response.get<tmt::UIComponent>();

    ImReflect::Input("Size", value.size, type_settings, type_response);
    {
        //type_settings.push<glm::vec2>().min({ -1.0f, -1.0f }).max({ 1.0f, 1.0f }).clamp();
        ImReflect::Input("Pivot", value.pivot, type_settings, type_response);
    }

    ImReflect::Input("Block Raycasts", value.block_raycasts, type_settings, type_response);
    ImReflect::Input("Scale With Parent X", value.scale_with_parent_x, type_settings, type_response);
    ImReflect::Input("Scale With Parent Y", value.scale_with_parent_y, type_settings, type_response);
    ImReflect::Input("Aspect Mode", value.aspect_mode, type_settings, type_response);
    ImReflect::Input("Aspect Ratio", value.aspect_ratio, type_settings, type_response);
    ImResponse anchor_response = ImReflect::Input("Anchor", value.anchor);

    if (anchor_response.get<tmt::Anchor>().is_changed()) {
        switch (value.anchor) {
            case tmt::Anchor::TOP_LEFT:
                value.pivot = { 0.0f, 0.0f };
                break;
            case tmt::Anchor::TOP_CENTER:
                value.pivot = { 0.5f, 0.0f };
                break;
            case tmt::Anchor::TOP_RIGHT:
                value.pivot = { 1.0f, 0.0f };
                break;
            case tmt::Anchor::MIDDLE_LEFT:
                value.pivot = { 0.0f, 0.5f };
                break;
            case tmt::Anchor::MIDDLE_CENTER:
                value.pivot = { 0.5f, 0.5f };
                break;
            case tmt::Anchor::MIDDLE_RIGHT:
                value.pivot = { 1.0f, 0.5f };
                break;
            case tmt::Anchor::BOTTOM_LEFT:
                value.pivot = { 0.0f, 1.0f };
                break;
            case tmt::Anchor::BOTTOM_CENTER:
                value.pivot = { 0.5f, 1.0f };
                break;
            case tmt::Anchor::BOTTOM_RIGHT:
                value.pivot = { 1.0f, 1.0f };
                break;
        }
    }
}
