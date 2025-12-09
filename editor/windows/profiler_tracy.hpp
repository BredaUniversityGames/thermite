#pragma once

#include "editor/core/window.hpp"

namespace tmt {
// This implementation is windows specific
class Profiler : public IWindow {
   public:
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
    void display() override;
    constexpr std::string get_title() const override { return "Profiler"; }
};
}  // namespace tmt
