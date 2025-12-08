#include "game_flow.hpp"
#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

void GameFlow::display() {
    if (ImGui::Button("Start")) {
        engine.game_controller.start_game();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        engine.game_controller.pause_game();
    }
    ImGui::SameLine();
    if (ImGui::Button("Resume")) {
        engine.game_controller.resume_game();
    }
    ImGui::SameLine();
    if (ImGui::Button("End")) {
        engine.game_controller.end_game();
        engine.ecs.clear();
    }
}

void GameFlow::on_editor_start() {}

void GameFlow::on_editor_update(const FrameData&) {}

void GameFlow::on_editor_end() {}

}  // namespace tmt