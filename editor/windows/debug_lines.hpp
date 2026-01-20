#pragma once
#include "editor/core/window.hpp"
#include "engine/events/debug.hpp"

namespace tmt {
class DebugLines : public IWindow, public OnDrawLines {
   public:
    void display() override;

    virtual void on_editor_update(const tmt::FrameData& time) override;

    void on_draw_lines() override;

    constexpr std::string get_title() const override { return "Debug Lines"; };
    constexpr std::string get_name() const override { return "Test"; };
    constexpr bool default_open() const override { return false; }
};
}  // namespace tmt