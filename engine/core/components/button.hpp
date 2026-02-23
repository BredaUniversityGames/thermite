#pragma once
#include "engine/tools/types/color.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

enum class ButtonState { Idle, OnSelect, Selected, OnDeselect, Disabled, OnClick, OnHold, OnRelease };
// enum class Direction { NorthWest, North, NorthEast, East, SouthEast, South, SouthWest, West };

template <typename... Args>
struct ButtonCallback {
    std::vector<std::function<void(Args...)>> callbacks;

    ButtonCallback() = default;
    ButtonCallback(std::function<void(Args...)> callback) { callbacks.push_back(callback); }

    void operator()(Args... args) {
        for (auto& callback : callbacks) {
            callback(args...);
        }
    };

    void operator+=(std::function<void(Args...)> callback) { callbacks.push_back(callback); }
    void operator=(std::function<void(Args...)> callback) {
        callbacks.clear();
        callbacks.push_back(callback);
    }

    void add(std::function<void(Args...)> callback) { callbacks.push_back(callback); }
    void clear() { callbacks.clear(); }
};

struct Button {
   public:
    ButtonState state = ButtonState::Idle;
    ButtonCallback<> on_select = {};
    ButtonCallback<> on_selected = {};
    ButtonCallback<> on_deselect = {};
    ButtonCallback<> on_click = {};
    ButtonCallback<float> on_hold = {};
    ButtonCallback<> on_release = {};

    /* Color for each state */
    std::array<RGBA, 8> colors = { RGBA(glm::vec4(1.0f)), RGBA(glm::vec4(0.8f)), RGBA(glm::vec4(0.6f)), RGBA(glm::vec4(0.8f)),
                                   RGBA(glm::vec4(0.5f)), RGBA(glm::vec4(1.0f)), RGBA(glm::vec4(1.0f)), RGBA(glm::vec4(1.0f)) };
};

}  // namespace tmt

TMT_COMPONENT(tmt::Button, "Button", (colors));