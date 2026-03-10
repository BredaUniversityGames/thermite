#include "viewport.hpp"
#include <imgui.h>
#include <ImGuizmo.h>
#include "editor/editor.hpp"
#include "editor/gizmo.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "editor/events/scene.hpp"
#include "editor/windows/scenes.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/systems/motion_math/motion_math_system.hpp"

void tmt::Viewport::on_editor_start() {
    // Setup Input Actions
    register_input_actions();
}

void tmt::Viewport::register_input_actions() {
    engine.input_map.add_action_keys(Config::SPRINT, Key::LEFT_SHIFT);
    engine.input_map.add_action_keys(Config::FORWARD, Key::W);
    engine.input_map.add_action_keys(Config::BACKWARD, Key::S);
    engine.input_map.add_action_keys(Config::RIGHT, Key::D);
    engine.input_map.add_action_keys(Config::LEFT, Key::A);
    engine.input_map.add_action_keys(Config::UP, Key::E);
    engine.input_map.add_action_keys(Config::DOWN, Key::Q);
    engine.input_map.add_action_keys(Config::UP, Key::SPACE);
    engine.input_map.add_action_keys(Config::DOWN, Key::LEFT_CTRL);
}

void tmt::Viewport::on_editor_update(const tmt::FrameData& frame_data) {
    update_debug_camera(frame_data);
}

void tmt::Viewport::on_editor_end() {}

void tmt::Viewport::before_begin() {
    /*default min size*/
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_FirstUseEver);

    /* Zero margin */
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void tmt::Viewport::end_display() {
    ImGui::PopStyleVar();
}

std::string tmt::Viewport::get_title() const {
    const auto window_id = "###" ICON_MS_VISIBILITY " Viewport";
    const auto scene_name = engine.scenes.is_scene_loaded() ? engine.scenes.get_active_scene_info().name : "UNKNOWN";
    return scene_name + window_id;
}
int tmt::Viewport::get_window_flags() const {
    if (engine.game_controller.is_running()) return ImGuiWindowFlags_None;

    const bool is_scene_dirty = editor.windows[editor.editor_mode].get<ScenesWindow>().is_scene_dirty();
    return is_scene_dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
};

void tmt::Viewport::display() {
    auto size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f));
    width = size.x;
    height = size.y;

    const ImVec2 img_cursor_pos = ImGui::GetCursorScreenPos();
    image_pos = {img_cursor_pos.x, img_cursor_pos.y};
	
    is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    if (height <= 0.f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    ImGui::BeginChild("viewport_render", ImVec2(0, 0), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Image((ImTextureRef)engine.renderer.render_view.imgui_viewport, size);
    is_hovered = ImGui::IsItemHovered();
    auto imgui_mouse_pos = ImGui::GetMousePos();
    mouse_pos.x = imgui_mouse_pos.x - image_pos.x;
    mouse_pos.y = imgui_mouse_pos.y - image_pos.y;

    left_panel();

    const std::vector<Entity>& selected_entities = editor.windows[editor.editor_mode].get<Hierarchy>().get_selected_entities();

    float snap_value = 0.0f;
    const bool ctrl_held = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift_held = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    const bool alt_held = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);

    if (ctrl_held) {
        const auto operation = Gizmo::OPERATIONS[editor.gizmo.operation];
        switch (operation) {
            case ImGuizmo::OPERATION::TRANSLATE:
                snap_value = editor.save_data.snap_values.move;
                break;
            case ImGuizmo::OPERATION::ROTATE:
                snap_value = editor.save_data.snap_values.rotation;
                break;
            case ImGuizmo::OPERATION::SCALE:
                snap_value = editor.save_data.snap_values.scale;
                break;
            case ImGuizmo::OPERATION::BOUNDS:
                snap_value = editor.save_data.snap_values.move;  // Use move snap for bounds
                break;
            default:
                break;
        }
    }

    const bool wants_to_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
    if (wants_to_capture_keyboard == false && using_debug_camera == false) {
        if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
            editor.gizmo.operation = 0;  // Translate
        } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            editor.gizmo.operation = 1;  // Rotate
        } else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            editor.gizmo.operation = 2;  // Scale
        } else if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
            editor.gizmo.operation = 3;  // Bounds
        } else if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
            snap_to_entity();
        }
    }

    // Build modifiers for bounds manipulation
    BoundsModifiers modifiers;
    modifiers.uniform_scale = shift_held;    // Shift: maintain aspect ratio
    modifiers.scale_from_center = alt_held;  // Alt: keep center fixed

    const bool gizmo_changed = editor.gizmo.manip(image_pos.x, image_pos.y, width, height, selected_entities, snap_value, modifiers);
    if (gizmo_changed) OnSceneModified::dispatch();

    const bool toolbar_buttons_hovered = toolbar(img_cursor_pos);

    selection_logic(imgui_mouse_pos, img_cursor_pos, toolbar_buttons_hovered);

    ImGui::EndChild();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGui::SetWindowFocus();
    }
    if(is_hovered && engine.input.get_game_preferred_mouse_lock() && engine.game_controller.is_running() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        engine.input.lock_mouse(true);
        engine.input.set_mouse_relative_to_window(true);
        engine.input.set_game_preferred_mouse_lock(false);
    }
}

void tmt::Viewport::snap_to_entity() {
    const auto* hierarchy = editor.windows[editor.editor_mode].try_get<Hierarchy>();
    Entity selected_entity = hierarchy->get_first_selected_entity();
    if (hierarchy == nullptr || !engine.ecs.valid(selected_entity)) return;

    auto& transform = engine.ecs.get_component<Transform>(selected_entity);
    glm::vec3 target_pos = transform.get_world_position();
    glm::vec3 target_scale = transform.get_world_scale();

    const auto* target_renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);

    float distance = 10.0f;  // fallback

    Camera& camera = engine.renderer.get_debug_camera();
    Transform& camTransform = engine.renderer.get_debug_transform();

    if (target_renderer && target_renderer->resource) {
        ResourceRef<VoxelVolume> volume = target_renderer->resource;

        if (volume) {
            // Get voxel grid size
            glm::vec3 voxelSize = glm::vec3(volume->size);

            // Apply world scale
            glm::vec3 worldSize = voxelSize * target_scale;

            glm::vec3 halfExtents = worldSize * 0.5f;

            float radius = glm::length(halfExtents);

            float fovRadians = glm::radians(camera.fov);

            distance = radius / std::tan(fovRadians * 0.5f);

            distance *= 0.2f;  // padding
        }
    }

    glm::vec3 forward = camTransform.get_forward();
    glm::vec3 newCamPos = target_pos - forward * distance;

    camTransform.set_world_position(newCamPos);
    camTransform.look_at(target_pos, glm::vec3(0, 1, 0));

    // Sync yaw/pitch
    glm::vec3 dir = glm::normalize(target_pos - newCamPos);
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));
    camera.pitch = glm::degrees(asin(dir.y));
}

bool tmt::Viewport::toolbar(const ImVec2& image_pos) {
    ImGuiStyle& style = ImGui::GetStyle();

    float btn_w = ImGui::CalcTextSize(ICON_MS_LANGUAGE).x + style.FramePadding.x * 2.0f;
    float btn_h = ImGui::GetFrameHeight();
    float total_w = btn_w * 3.0f + style.ItemSpacing.x;

    // Small padding from the image border
    float pad = 4.0f;
    ImVec2 btn_pos = ImVec2(image_pos.x + width - total_w - pad, image_pos.y + pad);

    bool any_button_hovered = false;

    ImGui::SetCursorScreenPos(btn_pos);
    const char* space_button_icon = editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR;
    if (ImGui::Button(space_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);
    }
    any_button_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine();
    const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
    if (ImGui::Button(mode_button_icon, ImVec2(btn_w, btn_h))) {
        ++editor.gizmo.operation;
        if (editor.gizmo.operation == Gizmo::GIZMO_OP_COUNT) editor.gizmo.operation = 0;
    }
    any_button_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine();
    const char* multi_button_icon = editor.gizmo.multiselect_mode ? ICON_MS_FILTER_NONE : ICON_MS_FILTER_1;
    if (ImGui::Button(multi_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.multiselect_mode = static_cast<uint8_t>(!editor.gizmo.multiselect_mode);
    }
    any_button_hovered |= ImGui::IsItemHovered();

    return any_button_hovered;
}

void tmt::Viewport::selection_logic(const ImVec2& imgui_mouse_pos, const ImVec2& image_pos, bool toolbar_buttons_hovered) {
    if (!is_hovered || !engine.game_controller.is_viewing_scene()) return;

    auto& hierarchy = editor.windows[Editor::Mode::SCENE].get<Hierarchy>();
    const std::vector<Entity>& selected_entities = hierarchy.get_selected_entities();
    auto& imgui_io = ImGui::GetIO();

    static ImVec2 first_click_pos;
    static bool using_rect = false;

    bool not_multiple_select_modifier = !imgui_io.KeyCtrl && !imgui_io.KeyShift;

    if (imgui_io.MouseReleased[0]) using_rect = false;
    if (imgui_io.MouseDown[0] && (!editor.gizmo.hovered() || using_rect)) {
        if (imgui_io.MouseClicked[0]) {
            first_click_pos = imgui_io.MousePos;
        }
        ImVec2 delta = imgui_io.MousePos - first_click_pos;

        // if delta has moved by 5 pixels, start rectangle logic
        if (abs(delta.x) + abs(delta.y) > 5) {
            using_rect = true;

            ImGui::GetWindowDrawList()->AddRectFilled(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.2f));
            ImGui::GetWindowDrawList()->AddRect(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.6f));

            glm::ivec2 a = { first_click_pos.x - image_pos.x, first_click_pos.y - image_pos.y };
            glm::ivec2 b = { imgui_mouse_pos.x - image_pos.x, imgui_mouse_pos.y - image_pos.y };

            int x0 = std::min(a.x, b.x);
            int x1 = std::max(a.x, b.x);
            int y0 = std::min(a.y, b.y);
            int y1 = std::max(a.y, b.y);

            auto view = engine.ecs.view<UIComponent>();
            static std::vector<Entity> tracking_rect_entities;
            tracking_rect_entities.clear();
            for (int x = x0; x < x1; x += 5) {
                for (int y = y0; y < y1; y += 5) {
                    const tmt::Ray selection_ray = tmt::engine.renderer.render_view.pixel_ray({ x, y });
                    const tmt::Hit hit = tmt::engine.renderer.trace_ray(selection_ray);

                    if (!hit.miss()) {
                        tracking_rect_entities.push_back(hit.entity);
                        hierarchy.add_entity_to_selection(hit.entity);
                    }
                }
            }

            // cleanup when area shrinks
            if (not_multiple_select_modifier) {
                for (auto entity : selected_entities) {
                    if (std::find(tracking_rect_entities.begin(), tracking_rect_entities.end(), entity) == tracking_rect_entities.end()) {
                        hierarchy.remove_entity_from_selection(entity);
                    }
                }
            }
        }
    }
    if (imgui_io.MouseClicked[0] && !editor.gizmo.hovered() && !toolbar_buttons_hovered) {
        const tmt::Ray selection_ray = tmt::engine.renderer.render_view.pixel_ray({ mouse_pos.x, mouse_pos.y });
        const tmt::Hit hit = tmt::engine.renderer.trace_ray(selection_ray);

        if (not_multiple_select_modifier) hierarchy.clear_selection();

        if (engine.renderer.ui_pipeline.render_ui_pipeline) {
            auto view = engine.ecs.view<UIComponent>();
            for (auto [entity, ui_comp] : view.each()) {
                if (AnchorHelper::is_inside(entity, { mouse_pos.x, mouse_pos.y })) {
                    hierarchy.add_entity_to_selection(entity);
                    return;
                }
            }
        }

        if (!hit.miss()) {
            if (hierarchy.is_entity_selected(hit.entity) && !(not_multiple_select_modifier))
                hierarchy.remove_entity_from_selection(hit.entity);
            else {
                hierarchy.add_entity_to_selection(hit.entity);
                Entity first_entity = hierarchy.get_first_selected_entity();

                // only care about opening the first selected
                if (first_entity == hit.entity) {
                    auto& transform = engine.ecs.get_component<Transform>(hit.entity);

                    // we only want to open the tree if the selected entity has a parent
                    if (transform.has_parent()) {
                        Entity parent = transform.get_parent();
                        force_open_recurse_upwards(parent);
                    }
                }
            }
        }
    }
}

void tmt::Viewport::left_panel() {

    auto& mm = engine.ecs.systems.get<MotionMathSystem>();

    ImGuiIO& io = ImGui::GetIO();

    // Persistent state per panel (use a map keyed by id if you have multiple)
    //static float t = 0.0f;  // 0 closed, 1 open
    static bool pinned_open = false;

    // Child rect in screen space
    const ImVec2 childMin = ImGui::GetWindowPos();
    const ImVec2 childSize = ImGui::GetWindowSize();
    const ImVec2 childMax = ImVec2(childMin.x + childSize.x, childMin.y + childSize.y);

    // "Edge hot zone" like Blender: hover near left edge to open
    const float edge_zone = 10.0f;      // px
    const float grab_handle_w = 14.0f;  // visible handle thickness
    const float panel_h = childSize.y;

    const bool mouse_in_child = io.MousePos.x >= childMin.x && io.MousePos.x <= childMax.x && io.MousePos.y >= childMin.y && io.MousePos.y <= childMax.y;

    const bool hover_edge = mouse_in_child && (io.MousePos.x <= childMin.x + edge_zone);

    // Also keep it open while interacting with it
    // We'll set this after we build the panel window via IsWindowHovered/IsWindowFocused.
    bool interacting_with_panel = false;

    // Desired state
    bool want_open = pinned_open || hover_edge;

    static SecondOrderSolver::State<float> t_state {.current_state = 0.f};
    
    SecondOrderSolver::solve(t_state, want_open ? 1.f : 0.f, 1.f, 1.f, 0.f, tmt::engine.frame_data().delta_time);
    float t = t_state.current_state;

    //t = Approach(t, want_open ? 1.0f : 0.0f, want_open ? open_speed : close_speed, io.DeltaTime);



    // Early out if basically closed (optional)
    if (t < 0.001f && !want_open) t = 0.0f;

    // Eased
    //const float te = EaseOutCubic(t);

    // Panel position: slides in from left
    // When closed: x = childMin.x - panel_width
    // When open:   x = childMin.x
    const float x = glm::mix(childMin.x - 370.f, childMin.x, t);
    const ImVec2 panelPos = ImVec2(x, childMin.y);
    const ImVec2 panelSize = ImVec2(370.f, panel_h);

    // Handle area (always slightly visible when closed? Blender shows a tiny tab)
    // We'll draw a tab that sits on the left edge; when closed, it sits at childMin.x.
    const float tab_x = (t > 0.01f) ? (panelPos.x + panelSize.x - grab_handle_w) : (childMin.x);
    const ImVec2 tabMin = ImVec2(tab_x, childMin.y + 40.0f);
    const ImVec2 tabMax = ImVec2(tab_x + grab_handle_w, childMin.y + 120.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Visuals: panel background + border (drawn behind the panel window contents)
    if (t > 0.001f) {
        const ImU32 bg = ImGui::GetColorU32(ImGuiCol_WindowBg);
        const ImU32 bd = ImGui::GetColorU32(ImGuiCol_Border);
        dl->AddRectFilled(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y), bg, 6.0f);
        dl->AddRect(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y), bd, 6.0f);
    }

    // Tab visuals
    {
        const ImU32 tabCol = ImGui::GetColorU32(ImGuiCol_Button);
        const ImU32 tabColH = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        bool tabHovered = mouse_in_child && io.MousePos.x >= tabMin.x && io.MousePos.x <= tabMax.x && io.MousePos.y >= tabMin.y && io.MousePos.y <= tabMax.y;

        dl->AddRectFilled(tabMin, tabMax, tabHovered ? tabColH : tabCol, 6.0f);
        dl->AddRect(tabMin, tabMax, ImGui::GetColorU32(ImGuiCol_Border), 6.0f);

        // Small "grip" lines
        const float cx = (tabMin.x + tabMax.x) * 0.5f;
        dl->AddLine(ImVec2(cx - 3, tabMin.y + 20), ImVec2(cx - 3, tabMax.y - 20), ImGui::GetColorU32(ImGuiCol_TextDisabled), 1.0f);
        dl->AddLine(ImVec2(cx + 3, tabMin.y + 20), ImVec2(cx + 3, tabMax.y - 20), ImGui::GetColorU32(ImGuiCol_TextDisabled), 1.0f);

        // Click tab to pin/unpin
        if (tabHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) pinned_open = !pinned_open;
    }

    // Actual panel UI as a borderless window positioned over the overlay rect
    // Use a unique name (panel_id) so Dear ImGui can track it.
    if (t > 0.001f) {
        ImGui::SetNextWindowPos(panelPos);
        ImGui::SetNextWindowSize(panelSize);
        ImGui::SetNextWindowBgAlpha(0.0f);  // we already drew bg in draw list; keep window transparent

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;

        // IMPORTANT: prevent the panel from extending outside child interaction-wise
        // We'll clip by pushing a clip rect (draw + input). Draw list clip is enough for visuals;
        // input still works fine as long as panel rect is within child rect (it is).
        ImVec2 min = ImVec2(image_pos.x, image_pos.y);
        ImGui::PushClipRect(min, min + ImVec2(width, height), false);

        ImGui::Begin("panel_", nullptr, flags);
        interacting_with_panel = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        // Content
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextUnformatted("Viewport Tools");
        ImGui::Separator();

        static float exposure = 1.0f;
        static int mode = 0;
        ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f);
        ImGui::RadioButton("Select", &mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Move", &mode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Rotate", &mode, 2);

        ImGui::End();
        ImGui::PopClipRect();
    }

    // If user is using the panel, keep it open even if they move away from edge.
    // (Optional) This mimics Blender nicely.
    if (interacting_with_panel) pinned_open = true;  // don’t auto-unpin; just ensures it stays open during interaction
    else pinned_open = false;
}

void tmt::Viewport::force_open_recurse_upwards(Entity entity) {
    auto& hierarchy = editor.windows[Editor::Mode::SCENE].get<Hierarchy>();

    hierarchy.add_entity_to_forced_open(entity);

    auto& transform = engine.ecs.get_component<Transform>(entity);
    if (transform.has_parent()) {
        Entity parent = transform.get_parent();
        force_open_recurse_upwards(parent);
    }
}

void tmt::Viewport::on_retrieve_mouse_state(MouseOverride& event) {
    event.x = mouse_pos.x;
    event.y = mouse_pos.y;
    event.handled = true;
}

void tmt::Viewport::on_block_input_request(OnBlockInputEvent& event) {
    if (is_focused == false) {
        event.block = true;
    }
    event.handled = true;
}

void tmt::Viewport::on_game_start() 
{
   ImGui::SetWindowFocus(get_title().c_str());
}

void tmt::Viewport::update_debug_camera(const tmt::FrameData& time) {
    if (!engine.game_controller.is_viewing_scene() || is_hovered == false) return;
    // Gather Variables to be used
    auto& input = engine.input;

    const bool enable_mouse_look = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool is_2d_axis_movement = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (!enable_mouse_look) {
        /* Show mouse cursor */

        input.set_mouse_relative_to_window(false);
        input.lock_mouse(false);
        using_debug_camera = false;
        return;
    }
    using_debug_camera = true;
    /* Cancel any text fields in editor you may interact with */
    ImGui::SetWindowFocus();

    /* Hide mouse cursor */
    input.set_mouse_relative_to_window(true);
    input.lock_mouse(true);

    Camera& camera = engine.renderer.get_debug_camera();
    Transform& transform = engine.renderer.get_debug_transform();

    // const float dx = ImGui::GetIO().MouseDelta.x;
    // const float dy = ImGui::GetIO().MouseDelta.y;
    // printf("Mouse Delta: %f, %f\n", dx, dy);
    const float dx = input.get_mouse_delta_engine_x();
    const float dy = input.get_mouse_delta_engine_y();

    if (is_2d_axis_movement == false) {
        // Mouse Look
        camera.yaw -= dx * Config::MOUSE_SENSITIVITY;
        camera.pitch -= dy * Config::MOUSE_SENSITIVITY;
        camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

        glm::vec3 front = {};
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front = glm::normalize(front);
        transform.look_at(transform.get_world_position() + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // const float mouse_wheel_y_delta = input.get_mouse_wheel_y();
    const float mouse_wheel_y_delta = ImGui::GetIO().MouseWheel;
    camera_speed *= std::pow(2.0f, mouse_wheel_y_delta * 0.15f);
    camera_speed = glm::clamp(camera_speed, Config::MIN_BASE_SPEED, Config::MAX_BASE_SPEED);

    glm::vec3 pos = transform.get_world_position();

    if (is_2d_axis_movement) {
        // 2D Axis Movement
        const bool sprint = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
        const glm::vec3 horizontal_move = transform.get_right() * dx * Config::MOUSE_SENSITIVITY * 0.1f;
        const glm::vec3 direction = sprint ? transform.get_forward() : transform.get_up();
        const glm::vec3 vertical_move = direction * -dy * Config::MOUSE_SENSITIVITY * 0.1f;
        pos += (horizontal_move + vertical_move);
    } else {
        // Movement (WASD + QE)
        glm::vec3 move_dir = { 0.0f, 0.0f, 0.0f };
        if (ImGui::IsKeyDown(ImGuiKey_W)) move_dir += transform.get_forward();
        if (ImGui::IsKeyDown(ImGuiKey_S)) move_dir -= transform.get_forward();
        if (ImGui::IsKeyDown(ImGuiKey_A)) move_dir -= transform.get_right();
        if (ImGui::IsKeyDown(ImGuiKey_D)) move_dir += transform.get_right();
        if (ImGui::IsKeyDown(ImGuiKey_E)) move_dir += transform.get_up();
        if (ImGui::IsKeyDown(ImGuiKey_Q)) move_dir -= transform.get_up();

        if (glm::length(move_dir) > 0.0f) pos += glm::normalize(move_dir) * camera_speed * time.delta_time;
    }

    transform.set_world_position(pos);
}
