#include "window.hpp"

#include <SDL3/SDL.h>

namespace tmt {

Window::~Window() {
    if (window && SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_DestroyWindow(window);
    }
}

void Window::init() {
    SDL_SetAppMetadata("Thermite Engine", "0.1", "com.thermite.engine");

   if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_VULKAN);

    if (!window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return;
    }
}

void Window::update() {
    SDL_Event event {};

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT: { is_running = false; }
        }
    }
}

HWND Window::get_window_handle() {
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    // Not sure whether it sets any errors when returning a default value, so
    // this may be redundant
    if (!hwnd) {
        SDL_Log("Failed to get HWND: %s", SDL_GetError());
        return nullptr;
    } else {
        SDL_Log("HWND: %p", hwnd);
    }
    return hwnd;
}

}  // namespace tmt
