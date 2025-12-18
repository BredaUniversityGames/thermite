#include "input_map.hpp"

using namespace tmt;
void InputMap::setup_default_actions() {
    add_action(action::CONFIRM);
    add_action_keys(action::CONFIRM, Key::SPACE, Key::RETURN);
    add_action_gamepad_buttons(action::CONFIRM, GamepadButton::SOUTH);

    add_action(action::CANCEL);
    add_action_keys(action::CANCEL, Key::ESCAPE);
    add_action_gamepad_buttons(action::CANCEL, GamepadButton::EAST);

    add_action(action::LEFT_CLICK);
    add_action_mouse(action::LEFT_CLICK, MouseButton::LEFT);
    add_action_gamepad_buttons(action::LEFT_CLICK, GamepadButton::RIGHT_SHOULDER);

    add_action(action::RIGHT_CLICK);
    add_action_mouse(action::RIGHT_CLICK, MouseButton::RIGHT);
    add_action_gamepad_buttons(action::RIGHT_CLICK, GamepadButton::LEFT_SHOULDER);

    add_action(action::MIDDLE_CLICK);
    add_action_mouse(action::MIDDLE_CLICK, MouseButton::MIDDLE);
    add_action_gamepad_buttons(action::MIDDLE_CLICK, GamepadButton::RIGHT_STICK);

    add_action(action::MOUSE_MOTION);
    add_action_mouse_motion(action::MOUSE_MOTION);

    add_action(action::GAMEPAD_LEFT_STICK);
    add_action_gamepad_axes(action::GAMEPAD_LEFT_STICK, GamepadAxis::LEFT_X, GamepadAxis::LEFT_Y);

    add_action(action::GAMEPAD_RIGHT_STICK);
    add_action_gamepad_axes(action::GAMEPAD_RIGHT_STICK, GamepadAxis::RIGHT_X, GamepadAxis::RIGHT_Y);

    add_action(action::GAMEPAD_TRIGGERS);
    add_action_gamepad_axes(action::GAMEPAD_TRIGGERS, GamepadAxis::LEFT_TRIGGER, GamepadAxis::RIGHT_TRIGGER);
}

void InputMap::add_action(const std::string& name) { actions[name] = InputAction {}; }

void InputMap::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }

void InputMap::add_key_to_action(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void InputMap::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void InputMap::add_action_mouse_motion(const std::string& name) { add_action_event(name, std::make_unique<InputEventMouseMotion>()); }

void InputMap::remove_action(const std::string& name) { actions.erase(name); }

const InputAction* InputMap::get_action(std::string_view name) const {
    auto it = actions.find(std::string {name});
    if (it == actions.end()) return nullptr;
    return &it->second;
}

bool InputMap::has_action(std::string_view name) const { return actions.find(std::string {name}) != actions.end(); }

float tmt::InputMap::get_action_deadzone(std::string_view name) const {
    const auto* action = get_action(name);
    return action ? action->deadzone : 0.0f;
}
