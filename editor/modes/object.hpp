#pragma once

#include "editor/core/mode.hpp"

#include "engine/events/scene.hpp"
#include "engine/tools/scene_types.hpp"

namespace tmt {

class ObjectMode : public IEditorMode, public OnPreLoadScene {
   public:
    ObjectMode();

    // Inherited via IEditorMode.
    constexpr std::string get_name() override { return "Object"; }
    void display_main_menu() override;
    void on_switch_to() override;
    void on_switch_away() override;

   private:
    // Inherited via OnPreLoadScene.
    void on_pre_load_scene(PreLoadSceneEvent& event) override;

    SceneIndex previous_scene_type;
    nlohmann::json cached_scene {};

    // Used as a flag for the on_pre_load_scene function call.
    bool is_switching_to {false};
};

}  // namespace tmt