#pragma once

#include "editor/core/window.hpp"

#include <imgui.h>

namespace tmt {

class VoxelEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_DEPLOYED_CODE " Voxel Editor"; }
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_NoMove; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}
};
}  // namespace tmt