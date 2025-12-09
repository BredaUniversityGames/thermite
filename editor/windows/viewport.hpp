#pragma once
#include "editor/core/window.hpp"

namespace tmt{
class Viewport : public IWindow
{
    Viewport() = default;
    ~Viewport() = default;

    // Inherited via IWindow
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
    void display() override;

    constexpr std::string get_title() const override { return "Viewport"; };

};
} // namespace tmt