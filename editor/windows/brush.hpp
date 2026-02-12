#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class Brush : public IWindow {
   public:
    enum class Tool : uint8_t { MULTI_TOOL, COLOR_PICKER, SINGLE, BOX };
    enum class Mode : uint8_t { ATTACH, REMOVE, PAINT };

    Brush() = default;

    [[nodiscard]] Tool get_active_tool() const { return active_tool; }
    [[nodiscard]] Mode get_active_mode() const { return active_mode; }

   private:
    constexpr std::string get_title() const override { return ICON_MS_BRUSH " Brushes"; }
    constexpr bool default_open() const override { return true; }

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

    void display() override;

    static const std::unordered_map<Tool, const char*> MODE_ICONS;
    Tool active_tool;
    Mode active_mode;
};

}  // namespace tmt