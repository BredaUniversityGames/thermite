#pragma once

#include "engine/core/keys.hpp"
namespace tmt {
class Input;

class InputEvent {
   public:
    virtual ~InputEvent() = default;
    virtual bool is_pressed() const = 0;
    virtual bool is_just_pressed() const = 0;
    virtual bool is_just_released() const = 0;
};

class InputEventKey : public InputEvent {
   public:
    explicit InputEventKey(Key key) : key(key) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;

    Key key {Key::UNKNOWN};
};

enum class MouseButton : int32_t {
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    X1 = 4,
    X2 = 5,
};

class InputEventMouseButton : public InputEvent {
   public:
    explicit InputEventMouseButton(MouseButton button) : button(button) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;

    MouseButton button;
};
}  // namespace tmt
