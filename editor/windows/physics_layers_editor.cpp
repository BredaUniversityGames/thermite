#include "physics_layers_editor.hpp"
#include "engine/engine.hpp"
#include "engine/systems/physics/physics_system.hpp"

#include "engine/core/logger.hpp"

#include <imgui.h>

namespace tmt {

void PhysicsLayersEditor::on_editor_start() {
    // Load layers from disk when editor opens
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (physics) {
        physics->layers().load();
    }
}

void PhysicsLayersEditor::on_editor_end() {
    // Optional: save on close
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (physics) {
        physics->layers().save();
    }
}

struct PhysicsLayerEditorState {
    char new_layer_name[64] {};
    char rename_buffer[64] {};
    int selected_layer = -1;
};

static PhysicsLayerEditorState g_state;

void PhysicsLayersEditor::display() {
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (!physics) return;

    PhysicsLayers& layers = physics->layers();

    ImGui::Begin("Physics Layers");

    ImGui::BeginChild("LayerList", ImVec2(250, 0), true);
    draw_layer_list(layers);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CollisionMatrix", ImVec2(0, 0), true);
    draw_collision_matrix(layers);
    ImGui::EndChild();

    ImGui::End();
}

void PhysicsLayersEditor::draw_layer_list(PhysicsLayers& layers) {
    ImGui::Text("Layers");
    ImGui::Separator();

    for (uint32_t i = 0; i < layers.get_layer_count(); ++i) {
        const std::string& name = layers.get_layer_name(i);
        if (name.empty()) continue;

        if (ImGui::Selectable(name.c_str(), g_state.selected_layer == (int)i)) {
            g_state.selected_layer = i;

            // Copy current name into rename buffer
            strncpy(g_state.rename_buffer, name.c_str(), sizeof(g_state.rename_buffer) - 1);
            g_state.rename_buffer[sizeof(g_state.rename_buffer) - 1] = '\0';
        }
    }

    ImGui::Separator();

    // --- Rename Selected ---
    if (g_state.selected_layer >= 0) {
        ImGui::InputText("Rename", g_state.rename_buffer, 64);

        if (ImGui::Button("Apply Rename")) {
            if (g_state.rename_buffer[0]) {
                layers.set_layer_name(g_state.selected_layer, g_state.rename_buffer);
            }
        }

        if (ImGui::Button("Remove Selected")) {
            layers.remove_layer(g_state.selected_layer);
            g_state.selected_layer = -1;
        }

        ImGui::Separator();
    }

    // --- Add New ---
    ImGui::InputText("New Layer", g_state.new_layer_name, 64);

    if (ImGui::Button("Add Layer")) {
        if (g_state.new_layer_name[0]) {
            layers.add_layer(g_state.new_layer_name);
            g_state.new_layer_name[0] = '\0';
        }
    }
}

void PhysicsLayersEditor::draw_collision_matrix(PhysicsLayers& layers) {
    uint32_t count = layers.get_layer_count();

    if (count == 0) {
        ImGui::Text("No layers defined.");
        return;
    }

    ImGui::Text("Collision Matrix");
    ImGui::Separator();

    if (ImGui::BeginTable("CollisionTable", count + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
        // Header row
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text(" ");

        for (uint32_t col = 0; col < count; ++col) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", layers.get_layer_name(col).c_str());
        }

        for (uint32_t row = 0; row < count; ++row) {
            ImGui::TableNextRow();

            // Row header
            ImGui::TableNextColumn();
            ImGui::Text("%s", layers.get_layer_name(row).c_str());

            for (uint32_t col = 0; col < count; ++col) {
                ImGui::TableNextColumn();

                // Self collision: always enabled, not editable
                if (row == col) {
                    ImGui::Text("X");
                    continue;
                }

                // Lower triangle: don't duplicate UI
                if (col < row) {
                    ImGui::Text("-");
                    continue;
                }

                bool canCollide = layers.can_collide(row, col);

                ImGui::PushID(row * 100 + col);

                if (ImGui::Checkbox("", &canCollide)) {
                    layers.enable_collision(row, col, canCollide);
                }

                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }
}

}  // namespace tmt
