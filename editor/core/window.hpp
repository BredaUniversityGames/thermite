#pragma once
#include "editor/events/editor.hpp"
#include <string>
#include <imgui.h>

namespace tmt {
class IWindow : public IEditorEvents {
   public:
    virtual ~IWindow() = default;

    virtual void before_begin() {};
    virtual void display() = 0;
    virtual void end_display() {};

    constexpr virtual std::string get_title() const = 0;
    constexpr virtual ImGuiWindowFlags_ get_window_flags() const { return ImGuiWindowFlags_None; }
    constexpr virtual bool is_closable() const { return true; }
};
}  // namespace tmt