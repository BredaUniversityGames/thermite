#pragma once

#include "editor/core/window.hpp"
#include "engine/core/resource.hpp"

#include <imgui.h>

namespace tmt {

class VoxelVolume;

class NodeHierarchy : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_FLOWCHART " Node hierarchy"; }
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; }
    constexpr bool default_open() const override { return true; }

    void menu_bar();
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData& time) override {};
    void on_editor_end() override {};
};

}  // namespace tmt