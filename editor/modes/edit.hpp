#pragma once

#include "editor/core/mode.hpp"

#include "engine/core/scene.hpp"

namespace tmt {

class VoxelEditScene : public Scene<VoxelEditScene> {
   public:
    static constexpr std::string_view scene_name() { return "VoxelEditScene"; }
};

class EditMode : public IEditorMode {
   public:
    // Inherited via IEditorMode.
    constexpr std::string get_name() override { return "Edit"; }
    void on_switch_to() override;
    void on_switch_away() override;

   private:

    std::vector<char> edit_data;
};

}  // namespace tmt