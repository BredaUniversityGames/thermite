#include "input.hpp"

#include "engine.hpp"
#include "keys.hpp"
#include "logger.hpp"
#include "extern/SDL3/include/SDL3/SDL_events.h"
#include "extern/SDL3/include/SDL3/SDL_keyboard.h"
#include "extern/imgui/backends/imgui_impl_sdl3.h"
using namespace tmt;
void Input::init() {
    int32_t number_keys;
    keys = SDL_GetKeyboardState(&number_keys);
    prev_keys.resize(number_keys, false);
    setup_default_action();
}
void Input::update() {
    SDL_Event event {};

    std::copy_n(keys, prev_keys.size(), prev_keys.begin());
    prev_mouse_buttons = mouse_buttons;

    mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

    // Get current mouse state
    mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT: {
                engine.set_is_running(false);
                break;
            }
            default:
                break;
        }
    }
}
bool Input::is_key_pressed(Key key) const { return keys[to_sdl(key)]; }
bool Input::is_key_just_pressed(Key key) const {
    auto sdl_scancode = to_sdl(key);
    return keys[sdl_scancode] == true && prev_keys[sdl_scancode] == false;
}
bool Input::is_key_just_released(Key key) const {
    auto sdl_scancode = to_sdl(key);
    return keys[sdl_scancode] == false && prev_keys[sdl_scancode] == true;
}

bool Input::is_mouse_button_pressed(MouseButton button) const { return mouse_buttons & SDL_BUTTON_MASK(static_cast<int>(button)); }
bool Input::is_mouse_button_just_pressed(MouseButton button) const {
    auto mask = SDL_BUTTON_MASK(static_cast<int>(button));
    return (mouse_buttons & mask) == true && (prev_mouse_buttons & mask) == false;
}
bool Input::is_mouse_button_just_released(MouseButton button) const {
    auto mask = SDL_BUTTON_MASK(static_cast<int>(button));
    return (mouse_buttons & mask) == false && (prev_mouse_buttons & mask) == true;
}
void Input::add_key_to_action(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }
void Input::add_action_key(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void Input::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void Input::remove_action(const std::string& name) { actions.erase(name); }

bool Input::is_action_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_pressed() == true) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_pressed() == true) return true;
    }

    return false;
}

bool Input::is_action_just_released(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_released() == true) return true;
    }

    return false;
}
void Input::setup_default_action() {
    add_action("confirm", Key::RETURN, Key::SPACE);
    add_action("cancel", Key::ESCAPE);
}
