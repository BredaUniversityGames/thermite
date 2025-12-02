#pragma once
#include "keys.hpp"

#include <string>

#include "events/input_event.hpp"

#include <memory>
namespace tmt {

struct InputAction {
    std::vector<std::unique_ptr<InputEvent>> events;
};
class Input {
   public:
    // Rule of 0

    void init();
    void update();

    void add_action(const std::string& name);

    void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }

    void add_key_to_action(const std::string& name, Key key);
    void remove_action(const std::string& name);

    bool is_action_pressed(const std::string& name) const;
    bool is_action_just_pressed(const std::string& name) const;
    bool is_action_just_released(const std::string& name) const;

    bool is_keyboard_button_just_pressed(Key key) const;
    bool is_keyboard_button_pressed(Key key) const;
    bool is_keyboard_button_released(Key key) const;

    bool is_mouse_button_pressed(MouseButton button) const;
    bool is_mouse_button_just_pressed(MouseButton button) const;
    bool is_mouse_button_just_released(MouseButton button) const;

   private:
    void setup_default_action();
    // do not free this manually
    const bool* keys {nullptr};
    uint32_t mouse_buttons {0};
    uint32_t prev_mouse_buttons {0};
    float mouse_x {0};
    float mouse_y {0};
    // relative positions
    float mouse_dx {0};
    float mouse_dy {0};
    std::vector<bool> prev_keys;
    std::unordered_map<std::string, InputAction> actions;
};
}  // namespace tmt
