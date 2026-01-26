#include "object.hpp"

#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"

#include <imgui.h>

namespace tmt {

ObjectMode::ObjectMode() : previous_scene_type(engine.scenes.get_active_scene_type()) {}

void ObjectMode::display_main_menu() {
    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Save Scene")) {
            engine.scenes.serialize_active_scene();
        }
        ImGui::EndMenu();
    }
}

void ObjectMode::on_switch_to() {
    is_switching_to = true;
    engine.scenes.load_scene(previous_scene_type);
    is_switching_to = false;

    cached_scene.clear();
}

void ObjectMode::on_switch_away() {
    previous_scene_type = engine.scenes.get_active_scene_type();
    cached_scene = Serializer::serialize(engine.ecs);
}

void ObjectMode::on_pre_load_scene(PreLoadSceneEvent& event) {
    if (!is_switching_to) return;

    // Use the cached json to load the object mode scene instead of the default one, this restores the previous working state.
    event.scene_json = std::move(cached_scene);
    event.handled = true;
}

}  // namespace tmt