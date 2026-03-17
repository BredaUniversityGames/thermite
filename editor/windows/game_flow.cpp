#include "game_flow.hpp"
#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/tools/serializer/ecs.hpp"
#include "engine/core/input/input.hpp"

namespace tmt {

void GameFlow::pause_game() {
    engine.game_controller.pause_game();
    was_mouse_locked = engine.input.is_mouse_locked();
    unlock_mouse();
}

void GameFlow::resume_game() {
    engine.game_controller.resume_game();
    if (was_mouse_locked) {
        lock_mouse();
    }
}

void GameFlow::start_game() {
    working_scene = engine.scenes.get_active_scene_type();
    cached_scene = Serializer::serialize(engine.ecs);

    engine.game_controller.start_game();
}

void GameFlow::end_game() {
    engine.game_controller.end_game();
    has_ended = true;
}

void GameFlow::on_editor_start() {}

void GameFlow::on_editor_update(const FrameData&) {
    /* Release Mouse */
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        if (engine.input.is_mouse_locked()) {
            engine.input.set_game_preferred_mouse_lock(true);
            unlock_mouse();
        }
    }

    /* Ctrl + P to start / stop game */
    const bool ctrl_down = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool p_down = ImGui::IsKeyPressed(ImGuiKey_P, false);
    if (ctrl_down && p_down) {
        if (engine.game_controller.is_playing()) {
            end_game();
        } else {
            start_game();
        }
    }
}

void GameFlow::on_editor_end() {}

void GameFlow::unlock_mouse() {
    engine.input.lock_mouse(false);
    engine.input.set_mouse_relative_to_window(false);
}

void GameFlow::lock_mouse() {
    engine.input.lock_mouse(true);
    engine.input.set_mouse_relative_to_window(true);
}

void GameFlow::on_game_end() {
    unlock_mouse();

    if (has_ended == false) return;
    engine.scenes.enqueue_scene(working_scene);
}

void GameFlow::on_pre_load_scene(PreLoadSceneEvent& event) {
    engine.input.set_game_preferred_mouse_lock(false);  // reset

    if (has_ended == false) return;
    event.scene_json = cached_scene;
    event.handled = true;
    has_ended = false;
}

}  // namespace tmt