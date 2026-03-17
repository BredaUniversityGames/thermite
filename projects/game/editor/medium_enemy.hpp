#pragma once

#include <ImReflect.hpp>

#include "projects/game/components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* name, game::MediumEnemy& value, ImSettings& settings, ImResponse& response) {
    using namespace game;
    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);
    auto& type_settings = settings.get<MediumEnemy>();
    auto& type_response = response.get<MediumEnemy>();

    /* ── Entity References ───────────────────────────── */
    ImReflect::Input("Walkable Asteroid", value.walkable_asteroid, type_settings, type_response);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* ── Movement & Detection ────────────────────────── */
    if (ImGui::TreeNodeEx("Movement & Detection", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImReflect::Input("Walk Speed", value.walk_speed, type_settings, type_response);
        ImReflect::Input("Rotation Speed", value.rotation_speed, type_settings, type_response);
        ImReflect::Input("Height Above Ground", value.height_above_ground, type_settings, type_response);
        ImReflect::Input("Aggro Range", value.aggro_range, type_settings, type_response);
        ImGui::TreePop();
    }

    ImGui::Spacing();

    /* ── Attacks ─────────────────────────────────────── */
    if (ImGui::TreeNodeEx("Attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        /* Stomp */
        if (ImGui::TreeNode("Stomp")) {
            ImReflect::Input("Stomp Range", value.stomp_range, type_settings, type_response);
            ImGui::TreePop();
        }

        /* Missiles */
        if (ImGui::TreeNode("Missiles")) {
            ImReflect::Input("Missile Cooldown", value.missile_cooldown, type_settings, type_response);
            ImReflect::Input("Missile Burst", value.missile_burst, type_settings, type_response);
            ImReflect::Input("Burst Interval", value.burst_interval, type_settings, type_response);
            ImReflect::Input("Max Randomness", value.missile_max_randomness, type_settings, type_response);

            ImGui::Spacing();
            ImGui::TextDisabled("Per-Missile Settings");
            ImGui::Spacing();

            ImReflect::Input("Launch Speed", value.launch_speed, type_settings, type_response);
            ImReflect::Input("Stop Launching After", value.stop_launching_after, type_settings, type_response);
            ImReflect::Input("Home Speed", value.home_speed, type_settings, type_response);
            ImReflect::Input("Start Homing After", value.start_homing_after, type_settings, type_response);
            ImReflect::Input("Lifetime", value.life_time, type_settings, type_response);
            ImGui::TreePop();
        }

        /* Laser */
        if (ImGui::TreeNode("Laser")) {
            const char* mode_labels[] = { "Linear", "Exponential" };
            int mode = static_cast<int>(value.laser_mode);
            if (ImGui::Combo("Mode", &mode, mode_labels, IM_ARRAYSIZE(mode_labels))) {
                value.laser_mode = static_cast<MediumEnemy::Mode>(mode);
            }

            ImGui::Spacing();

            ImReflect::Input("Laser Range", value.laser_range, type_settings, type_response);
            ImReflect::Input("Laser Cooldown", value.laser_cooldown, type_settings, type_response);
            ImReflect::Input("Laser Duration", value.laser_duration, type_settings, type_response);
            ImReflect::Input("Accuracy", value.laser_accuracy, type_settings, type_response);
            ImReflect::Input("Max Randomness", value.laser_max_randomness, type_settings, type_response);
            ImReflect::Input("Prediction Length", value.laser_prediction_length, type_settings, type_response);
            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}