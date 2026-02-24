#pragma once
#include "engine/core/system.hpp"
#include "engine/events/input.hpp"
#include "engine/core/entity.hpp"
#include "engine/tools/types/direction.hpp"

namespace tmt {

class ButtonManager : public ISystem, public OnMouseMove, public OnMouseButton, public OnKeyboard, public OnGamepadButton, public OnGamepadAxis {
   public:
    std::string get_name() override { return "ButtonManager"; }
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void update_states(const tmt::FrameData& time);
    void on_end() override;

    void select_button(const Entity button);

   private:
    float hold_time = 0.0f;
    Entity selected_button = entt::null;

    // Inherited via OnMouseMove
    void on_mouse_move(MouseMoveEvent& event) override;

    // Inherited via OnMouseButton
    void on_mouse_button(MouseButtonEvent& event) override;

    // Inherited via OnKeyboard
    void on_keyboard(KeyboardEvent& event) override;

    // Inherited via OnGamepadButton
    void on_gamepad_button(GamepadButtonEvent& event) override;

    // Inherited via OnGamepadAxis
    void on_gamepad_axis(GamepadAxisEvent& event) override;
};

}  // namespace tmt