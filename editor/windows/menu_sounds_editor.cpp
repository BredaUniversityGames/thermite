#include "menu_sounds_editor.hpp"

#include "inspector.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ui/ui.hpp"
// #include "engine/systems/ui/ui_audio_registry.hpp"

#include <imgui.h>

namespace tmt {

void MenuSoundsEditor::on_editor_start() {}

void MenuSoundsEditor::on_editor_end() {}

void MenuSoundsEditor::on_inspect() {
    auto* audio = engine.ecs.systems.try_get<UI>();
    if (!audio) return;

    ImGui::Begin("Menu Sounds");

    if (ImGui::Button("Save")) audio->menu_sounds.save();

    Inspector::draw(audio->menu_sounds.sounds);

    ImGui::End();
}

}  // namespace tmt
