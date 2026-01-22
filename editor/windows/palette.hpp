#pragma once

#include "editor/core/window.hpp"

#include "engine/core/renderer/material.hpp"
#include <imgui.h>

namespace tmt {

struct VoxelSceneNode;

class Palette : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_PALETTE " Palette"; }
    constexpr bool default_open() const override { return true; }

    void before_begin() override;
    void end_display() override;
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    friend class ModelViewer;

    void display_palette(const VoxelSceneNode& node);
    void display_material_editor();

    MaterialIndex selected_material_index {0};

    struct Config {
        constexpr static inline glm::ivec2 GRID_SIZE { 8, 32 };
        constexpr static inline ImVec2 OUTSET { 2.0f, 2.0f };
    };
};

}  // namespace tmt