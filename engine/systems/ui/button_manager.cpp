#include "button_manager.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/ui_component.hpp"

namespace tmt {

void ButtonManager::on_start() {}

void ButtonManager::on_update(const tmt::FrameData& time) {
    const auto mouse_pos_delta = engine.input.get_mouse_delta();
    if (mouse_pos_delta != glm::vec2 { 0.0f }) {
        const auto mouse_pos = engine.input.get_mouse_position();
        auto view = engine.ecs.get_registry().view<Button>();
        for (auto [entity, button] : view.each()) {
            if (AnchorHelper::is_inside(entity, mouse_pos)) {
                select_button(entity);
                break;
            }
        }
        select_button(entt::null);
    }

    update_states(time);
}

void ButtonManager::update_states(const tmt::FrameData& time) {
    /* group with buttons and image renderers */
    auto group = engine.ecs.get_registry().group<Button>(entt::get<ImageRenderer>);
    for (const auto& [entity, button, image_renderer] : group.each()) {
        image_renderer.color = button.colors.at(static_cast<uint8_t>(button.state));

        switch (button.state) {
            case ButtonState::IDLE:
                break;
            case ButtonState::ON_SELECT:
                button.on_select();
                button.state = ButtonState::SELECTED;
                break;
            case ButtonState::SELECTED:
                button.on_selected();
                break;
            case ButtonState::ON_DESELECT:
                button.on_deselect();
                button.state = ButtonState::IDLE;
                break;
            case ButtonState::DISABLED:
                break;
            case ButtonState::ON_CLICK:
                button.on_click();
                button.state = ButtonState::ON_HOLD;
                break;
            case ButtonState::ON_HOLD:
                button.on_hold(time.delta_time);
                hold_time += time.delta_time;
                break;
            case ButtonState::ON_RELEASE:
                button.on_release();
                button.state = ButtonState::IDLE;
                hold_time = 0.0f;
                break;
        }
    }
}

void ButtonManager::on_end() {}

void ButtonManager::select_button(const Entity button) {
    if (selected_button == button) return;

    /* Deselect previous button */
    if (selected_button != entt::null) {
        auto& prev_button = engine.ecs.get_component<Button>(selected_button);
        prev_button.state = ButtonState::ON_DESELECT;
    }

    /* Select new button */
    selected_button = button;
    if (selected_button != entt::null) {
        auto& new_button = engine.ecs.get_component<Button>(selected_button);
        new_button.state = ButtonState::ON_SELECT;
    }
}

}  // namespace tmt