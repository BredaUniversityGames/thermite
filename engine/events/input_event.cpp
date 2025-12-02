#include "input_event.hpp"

#include "engine/engine.hpp"
#include "engine/core/input.hpp"
bool tmt::InputEventKey::is_pressed() const { return engine.input.is_keyboard_button_pressed(key); }
bool tmt::InputEventKey::is_just_pressed() const { return engine.input.is_keyboard_button_just_pressed(key); }
bool tmt::InputEventKey::is_just_released() const { return engine.input.is_keyboard_button_released(key); }
bool tmt::InputEventMouseButton::is_pressed() const { return engine.input.is_mouse_button_pressed(button); }
bool tmt::InputEventMouseButton::is_just_pressed() const { return engine.input.is_mouse_button_just_pressed(button); }
bool tmt::InputEventMouseButton::is_just_released() const { return engine.input.is_mouse_button_just_released(button); }
