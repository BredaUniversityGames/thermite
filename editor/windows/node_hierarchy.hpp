#pragma once

#include "editor/core/window.hpp"
#include "engine/core/resource.hpp"

#include <imgui.h>

#include "engine/core/resources/voxel_scene.hpp"

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
    void on_editor_update(const FrameData& time) override {}
    void on_editor_end() override {}

   private:
    friend class Palette;

    bool recurse_display_node(VoxelSceneNode& node);

    ResourceRef<VoxelScene> selected_scene;
    VoxelSceneNode* selected_node {nullptr};
};

}  // namespace tmt