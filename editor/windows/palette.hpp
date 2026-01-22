#pragma once

#include "editor/core/window.hpp"

#include "engine/core/renderer/material.hpp"

namespace tmt {

struct VoxelSceneNode;

class Palette : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_PALETTE " Palette"; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    friend class ModelViewer;

    void display_palette(const VoxelSceneNode& node);

    MaterialIndex selected_material_index{0};
};

}  // namespace tmt