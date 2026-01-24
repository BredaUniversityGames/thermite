#pragma once

#include "editor/core/mode.hpp"

#include "engine/core/entity.hpp"
#include "engine/core/scene.hpp"

namespace tmt {

struct Transform;
struct VoxelSceneNode;

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
    static Entity recurse_build_scene(VoxelSceneNode& node);
    static void recurse_parse_scene(Entity entity, const Transform& transform, VoxelSceneNode& node);

    std::vector<char> edit_data;
};

}  // namespace tmt