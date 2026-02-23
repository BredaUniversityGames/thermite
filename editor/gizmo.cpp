#include "gizmo.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/renderer/renderer.hpp"

#include "editor/windows/hierarchy.hpp"

#include "editor/events/scene.hpp"

#include "editor/core/systems/undo_redo/component_diff.hpp"

#include "engine/tools/serializer/all.hpp"

#include "engine/core/components/ui_component.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

namespace tmt {

const char* Gizmo::gizmo_op_icons[GIZMO_OP_COUNT] {
    ICON_MS_DRAG_PAN,
    ICON_MS_ROTATE_RIGHT,
    ICON_MS_ZOOM_OUT_MAP,
};

void Gizmo::init() {
    ImGuizmo::AllowAxisFlip(false);

    setup_style();
}

bool Gizmo::hovered() const {
    return ImGuizmo::IsOver();
}

bool Gizmo::manip(const float x, const float y, const float width, const float height, const std::span<const Entity>& selected_entities, const float snap) const {
    ImGuizmo::SetRect(x, y, width, height);
    ImGuizmo::SetDrawlist();

    if (engine.game_controller.is_running()) return false;

    if (selected_entities.empty()) {
        ImGuizmo::ResetOperation();
        return false;
    }

    bool has_ui = false;
    for (const Entity e : selected_entities) {
        if (!engine.ecs.valid(e)) continue;
        if (engine.ecs.has_component<UIComponent>(e)) {
            has_ui = true;
            break;
        }
    }

    const Transform& cam_transform = engine.renderer.get_debug_transform();
    const Camera& camera = engine.renderer.get_debug_camera();

    const glm::mat4 scene_view = glm::inverse(cam_transform.get_world_matrix());
    const glm::mat4 ui_view = glm::mat4(1.f);
    const glm::mat4 view = has_ui ? ui_view : scene_view;
    const float aspect = width / height;

    const glm::mat4 scene_perspective = glm::perspective(glm::radians(camera.fov), aspect, 0.05f, 1000.0f);
    const glm::mat4 ui_perspective = glm::ortho(0.0f, width, height, 0.0f, -1000.0f, 1000.0f);
    const glm::mat4 perspective = has_ui ? ui_perspective : scene_perspective;

    const Entity primary = selected_entities.front();
    if (!engine.ecs.valid(primary)) return false;

    // --- 1. Build the gizmo matrix ---
    const bool use_average = (multiselect_mode != 0);

    glm::mat4 gizmo_matrix;

    if (!use_average) {
        const auto wm = engine.ecs.get_component<Transform>(primary).get_world_matrix();
        const auto ui_offset = has_ui ? AnchorHelper::calculate_anchor_offset(primary) : glm::vec2(0.f);

        gizmo_matrix = wm;
        gizmo_matrix[3] += glm::vec4(ui_offset, 0.f, 0.f);
    } else {
        const float weight = 1.f / static_cast<float>(selected_entities.size());
        glm::vec3 avg_translation(0.f);
        glm::quat avg_rotation(0.f, 0.f, 0.f, 0.f);

        for (const Entity e : selected_entities) {
            const Transform& t = engine.ecs.get_component<Transform>(e);
            const glm::mat4& wm = t.get_world_matrix();
            const auto ui_offset = has_ui ? AnchorHelper::calculate_anchor_offset(e) : glm::vec2(0.f);

            avg_translation += glm::vec3(wm[3]) + glm::vec3(ui_offset, 0.f);
            avg_rotation += weight * t.get_world_rotation();
        }
        avg_translation /= static_cast<float>(selected_entities.size());
        avg_rotation = glm::normalize(avg_rotation);

        gizmo_matrix = glm::translate(glm::mat4(1.f), avg_translation) * glm::mat4_cast(avg_rotation) * glm::scale(glm::mat4(1.f), glm::vec3(1.f));
    }

    // --- 2. Manipulate ---
    glm::mat4 gizmo_matrix_before = gizmo_matrix;  // snapshot BEFORE manipulation

    glm::mat4 delta;
    ImGuizmo::SetOrthographic(has_ui);
    const bool changed = ImGuizmo::Manipulate(
        &view[0][0], &perspective[0][0], static_cast<ImGuizmo::OPERATION>(OPERATIONS[operation]), static_cast<ImGuizmo::MODE>(space), &gizmo_matrix[0][0], &delta[0][0],
        snap == 0.0f ? nullptr : &glm::vec3(snap)[0]
    );

    // --- 3. Undo/redo bookkeeping ---
    static std::unordered_map<Entity, ComponentDiff<Transform>> component_diffs;
    static bool was_using = false;
    const bool is_using = ImGuizmo::IsUsing();

    static glm::mat4 gizmo_start_matrix;
    static std::unordered_map<Entity, glm::mat4> entity_start_matrices;

    if (!was_using && is_using) {
        gizmo_start_matrix = gizmo_matrix_before;  // frozen gizmo reference
        for (const Entity e : selected_entities) {
            if (!engine.ecs.valid(e)) continue;
            entity_start_matrices[e] = engine.ecs.get_component<Transform>(e).get_world_matrix();

            ComponentDiff<Transform> diff(e);
            diff.before();
            component_diffs.emplace(e, std::move(diff));
        }
        was_using = true;
    }

    // --- 4. Apply delta ---
    if (changed) {
        // Delta from drag START, not from last frame
        const glm::mat4 own_delta = gizmo_matrix * glm::inverse(gizmo_start_matrix);

        for (const Entity e : selected_entities) {
            Transform& t = engine.ecs.get_component<Transform>(e);
            const glm::vec2 ui_offset = has_ui ? AnchorHelper::calculate_anchor_offset(e) : glm::vec2(0.f);

            if (!use_average && e == primary) {
                // Direct assignment is still cleanest for the primary
                glm::mat4 new_world = gizmo_matrix;
                new_world[3] -= glm::vec4(ui_offset, 0.f, 0.f);
                t.set_world_matrix(new_world);
            } else {
                // Apply delta to INITIAL world matrix, not current
                glm::mat4 effective_start = entity_start_matrices.count(e) ? entity_start_matrices[e] : t.get_world_matrix();

                effective_start[3] += glm::vec4(ui_offset, 0.f, 0.f);
                glm::mat4 new_effective = own_delta * effective_start;
                new_effective[3] -= glm::vec4(ui_offset, 0.f, 0.f);
                t.set_world_matrix(new_effective);
            }
        }
        OnSceneModified::dispatch();
    }

    // --- 5. Commit undo/redo ---
    if (!is_using && was_using) {
        UndoRedoCollection collection;
        for (auto& [e, diff] : component_diffs) {
            diff.after();
            collection.add_action(std::move(diff));
        }
        collection.commit("Edit Gizmo");
        component_diffs.clear();
        entity_start_matrices.clear();
        was_using = false;
    }

    return changed;
}

void Gizmo::setup_style() {
    // style..
    auto& style = ImGuizmo::GetStyle();
    style.TranslationLineThickness = 2.0f;
    style.TranslationLineArrowSize = 8.0f;
    style.RotationLineThickness = 3.0f;
    style.RotationOuterLineThickness = 3.0f;
    style.ScaleLineThickness = 2.0f;
    style.ScaleLineCircleSize = 1.5f;
    style.HatchedAxisLineThickness = 5.0f;
    style.CenterCircleSize = 24.0f;

    // Slightly neon, softer axis colors
    style.Colors[ImGuizmo::DIRECTION_X] = ImVec4(0.93725490f, 0.28235294f, 0.35686274f, 1.00f); /* red */
    style.Colors[ImGuizmo::DIRECTION_Y] = ImVec4(0.52941176f, 0.81176470f, 0.21176470f, 1.00f); /* green */
    style.Colors[ImGuizmo::DIRECTION_Z] = ImVec4(0.27843137f, 0.56078431f, 0.94509803f, 1.00f); /* blue */

    // Matching planes, with lower alpha
    style.Colors[ImGuizmo::PLANE_X] = ImVec4(0.93725490f, 0.28235294f, 0.35686274f, 0.4f);
    style.Colors[ImGuizmo::PLANE_Y] = ImVec4(0.52941176f, 0.81176470f, 0.21176470f, 0.4f);
    style.Colors[ImGuizmo::PLANE_Z] = ImVec4(0.27843137f, 0.56078431f, 0.94509803f, 0.4f);

    // Selection: warm golden accent
    style.Colors[ImGuizmo::SELECTION] = ImVec4(0.95686274f, 0.60392156f, 0.21960784f, 1.0f);

    // Inactive: cooler desaturated grey-blue
    style.Colors[ImGuizmo::INACTIVE] = ImVec4(0.32f, 0.35f, 0.42f, 0.85f);

    // Lines: slightly brighter/cleaner on dark background
    style.Colors[ImGuizmo::TRANSLATION_LINE] = ImVec4(0.60f, 0.63f, 0.70f, 0.80f);
    style.Colors[ImGuizmo::SCALE_LINE] = ImVec4(0.92f, 0.92f, 0.96f, 0.90f);

    // Rotation highlight: vivid magenta/orange mix
    style.Colors[ImGuizmo::ROTATION_USING_BORDER] = ImVec4(0.95686274f, 0.60392156f, 0.21960784f, 1.0f);
    style.Colors[ImGuizmo::ROTATION_USING_FILL] = ImVec4(0.95686274f, 0.60392156f, 0.21960784f, 0.4f);

    // Hatched axis lines: subtle, light on dark
    style.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);

    // Text: soft white with subtle shadow
    style.Colors[ImGuizmo::TEXT] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
    style.Colors[ImGuizmo::TEXT_SHADOW] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
}

}  // namespace tmt