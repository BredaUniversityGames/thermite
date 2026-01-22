#pragma once

#include "editor/core/window.hpp"

#include "engine/core/resource.hpp"
#include "engine/core/renderer/material.hpp"

namespace tmt {

class VoxelVolume;

class Palette : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_PALETTE " Palette"; }
    constexpr bool default_open() const override { return true; }

    [[nodiscard]] MaterialIndex get_selected_material_index() const { return selected_material_index; }

    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    void display_palette(const ResourceRef<VoxelVolume>& resource);

    MaterialIndex selected_material_index {0};
};

}  // namespace tmt