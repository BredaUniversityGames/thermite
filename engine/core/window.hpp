#pragma once

#include <string_view>

#include "events/engine.hpp"

class SDL_Window;

namespace tmt {

class Window : public OnEngineInit {
   public:
    SDL_Window* window = nullptr;

    int width = 1280;
    int height = 720;
    std::string_view title {};
    bool is_running = true;

    Window() = default;
    ~Window();

    void on_engine_init(const ApplicationSpecs& specs) override;
    void update();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

};

}  // namespace tmt