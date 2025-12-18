#pragma once
#include "input_event.hpp"

#include <memory>
#include <string>

namespace tmt {
namespace action {
constexpr const char* CONFIRM = "confirm";
constexpr const char* CANCEL = "cancel";
constexpr const char* LEFT_CLICK = "left_click";
constexpr const char* RIGHT_CLICK = "right_click";
constexpr const char* MIDDLE_CLICK = "middle_click";
constexpr const char* MOUSE_MOTION = "mouse_motion";
constexpr auto GAMEPAD_LEFT_STICK = "gamepad_left_stick";
constexpr auto GAMEPAD_RIGHT_STICK = "gamepad_right_stick";
constexpr auto GAMEPAD_TRIGGERS = "gamepad_triggers";
}  // namespace action
struct InputAction {
    std::vector<std::unique_ptr<InputEvent>> events;
    float deadzone = 0.5f;
};
class InputMap {
   public:
    void add_action(const std::string& name);
    template <typename... Keys>
    void add_action_keys(const std::string& name, Keys... keys) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventKey>(keys)), ...);
    }
    template <typename... Buttons>
    void add_action_gamepad_buttons(const std::string& name, Buttons... buttons) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadButton>(buttons)), ...);
    }
    template <typename... Axes>
    void add_action_gamepad_axes(const std::string& name, Axes... axes) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadMotion>(axes)), ...);
    }
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);
    void add_key_to_action(const std::string& name, Key key);
    void add_action_mouse(const std::string& name, MouseButton button);
    void add_action_mouse_motion(const std::string& name);
    void remove_action(const std::string& name);

    const InputAction* get_action(std::string_view name) const;
    bool has_action(std::string_view name) const;
    float get_action_deadzone(std::string_view name) const;

   private:
    // to not break the whole codebase, these are still tighly coupled
    friend class Input;
    void setup_default_actions();

    std::unordered_map<std::string, InputAction> actions {};
};
}  // namespace tmt
