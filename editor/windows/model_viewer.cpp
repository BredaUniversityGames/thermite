#include "model_viewer.hpp"

#include "editor.hpp"
#include "viewport.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/tools/file_dialog.hpp"

#include "editor/windows/palette.hpp"
#include "editor/windows/node_hierarchy.hpp"

namespace tmt {

namespace {

/* Ray AABB intersection function. */
float intersect_aabb(const Ray& ray, const glm::vec3 box_min, const glm::vec3 box_max) {
    const glm::vec3 t_to_min = (box_min - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_to_max = (box_max - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_min = glm::min(t_to_min, t_to_max);
    const glm::vec3 t_max = glm::max(t_to_min, t_to_max);
    const float t_near = glm::max(glm::max(glm::max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = glm::min(glm::min(t_max.x, t_max.y), t_max.z);

    return t_near > t_far ? BIG_F32 : t_far;
}

void draw_face_grid(const glm::vec3& start, const glm::vec3& right, const glm::vec3& up, const float length, const float height) {
    glm::vec3 point_one = start;
    glm::vec3 point_two = start + up * height;
    const glm::vec3 right_vector = right / static_cast<float>(VOXELS_PER_UNIT);
    const size_t x_line_count = static_cast<size_t>(std::ceil(length * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < x_line_count; i++) {
        point_one += right_vector;
        point_two += right_vector;

        engine.polyline.draw_line(point_one, point_two);
    }

    point_one = start;
    point_two = start + right * length;
    const glm::vec3 up_vector = up / static_cast<float>(VOXELS_PER_UNIT);
    const size_t y_line_count = static_cast<size_t>(std::ceil(height * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < y_line_count; i++) {
        point_one += up_vector;
        point_two += up_vector;

        engine.polyline.draw_line(point_one, point_two);
    }
}

struct FaceData {
    float normal_sign {0.0f};
    int32_t normal_index {-1};

    glm::vec3 world_normal {};
    glm::vec3 face_center {};
};

FaceData calculate_face_info(const int32_t face_index, const Transform& transform, const glm::vec3& half_extent) {
    const std::div_t& result = std::div(face_index, 2);

    glm::vec3 face_normal {};
    const float sign = (result.rem == 0 ? 1.0f : -1.0f);
    face_normal[result.quot] = sign;

    FaceData info {.normal_sign = sign, .normal_index = result.quot};
    info.world_normal = transform.get_world_matrix() * glm::vec4 {face_normal, 0.0f};
    info.face_center = transform.get_world_matrix() * glm::vec4 {face_normal * half_extent, 1.0f};

    return info;
}

bool draw_face(const FaceData& info, const Transform& transform, const glm::vec3& half_extent) {
    const glm::vec3 camera_position = engine.renderer.get_debug_transform().get_world_position();
    if (glm::dot(info.world_normal, camera_position - info.face_center) >= 0.0f) return false;

    glm::vec3 box_half_extent = half_extent;
    box_half_extent[info.normal_index] = 0.0f;

    engine.polyline.use_line_width(2.0f, true);
    engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 1.0f});

    engine.polyline.draw_obb(info.face_center, box_half_extent, transform.get_world_rotation());

    return true;
}

void draw_selected_face(const FaceData& info, const glm::vec3& half_extent, const Transform& transform) {
    glm::vec3 min = -half_extent;
    min[info.normal_index] = half_extent[info.normal_index] * info.normal_sign;
    min = transform.get_world_matrix() * glm::vec4 {min, 1.0f};

    engine.polyline.use_line_width(4.0f, false);
    engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 0.4f});
    switch (info.normal_index) {
        case 0:
            draw_face_grid(min, transform.get_forward(), transform.get_up(), half_extent[2] * 2.0f, half_extent[1] * 2.0f);
            break;

        case 1:
            draw_face_grid(min, transform.get_right(), transform.get_forward(), half_extent[0] * 2.0f, half_extent[2] * 2.0f);
            break;

        case 2:
            draw_face_grid(min, transform.get_right(), transform.get_up(), half_extent[0] * 2.0f, half_extent[1] * 2.0f);
            break;

        default:
            break;
    }
}

struct EditData {
    Hit hit;
    glm::vec3 half_extent;
};

void draw_selection(const EditData& edit_data, const Transform& transform) {
    engine.polyline.use_line_width(2.0f);
    engine.polyline.use_color(glm::vec4 {1.0f, 0.5f, 0.5f, 1.0f});

    const glm::vec3 local_voxel_pos = glm::vec3 {edit_data.hit.coord} / static_cast<float>(VOXELS_PER_UNIT) - edit_data.half_extent + VOXEL_SIZE_HALF;
    const glm::vec3 world_voxel_pos = transform.get_world_matrix() * glm::vec4 {local_voxel_pos, 1.0f};

    engine.polyline.draw_obb(world_voxel_pos, glm::vec3 {VOXEL_SIZE_HALF}, transform.get_world_rotation());
}

}  // namespace

void ModelViewer::display() {
    const ImVec2 content_start {0.0f, ImGui::GetFrameHeight()};
    ImGui::SetCursorPos(content_start);

    const ImVec2 size = ImGui::GetWindowSize() - content_start;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    ImGui::Image(engine.renderer.render_view.imgui_viewport, size);

    Viewport& viewport = editor.windows[Editor::Mode::OBJECT].get<Viewport>();

    viewport.is_hovered = ImGui::IsItemHovered();
    const ImVec2 mouse_pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
    mouse_position.x = viewport.mouse_pos.x = mouse_pos.x - content_start.x;
    mouse_position.y = viewport.mouse_pos.y = mouse_pos.y - content_start.y;
}

void ModelViewer::on_editor_update(const FrameData& time) {
    editor.windows[Editor::Mode::OBJECT].get<Viewport>().update_debug_camera(time);

    const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);

    const Entity selected_entity = editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().get_selected_entity();
    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);
    if (renderer == nullptr || !renderer->resource) return;

    EditData edit_data {
        .hit = engine.renderer.trace_ray(mouse_ray),
        .half_extent = glm::vec3 {renderer->resource->size} / static_cast<float>(2 * VOXELS_PER_UNIT),
    };

    const auto& transform = engine.ecs.get_component<Transform>(selected_entity);
    {
        const glm::mat4 world_to_local_matrix = glm::inverse(transform.get_world_matrix());

        const Ray local_ray {
            world_to_local_matrix * glm::vec4 {mouse_ray.origin, 1.0f},
            world_to_local_matrix * glm::vec4 {mouse_ray.dir, 0.0f},
        };

        const float far = intersect_aabb(local_ray, -edit_data.half_extent, edit_data.half_extent);
        const bool valid_hit = (far > 0.0f && edit_data.hit.distance >= far);

        for (int32_t i = 0; i < 6; i++) {
            FaceData face = calculate_face_info(i, transform, edit_data.half_extent);

            if (!draw_face(face, transform, edit_data.half_extent) || !valid_hit) continue;

            const glm::vec3 local_hit_position = local_ray.origin + local_ray.dir * far;
            const float extent_axis = edit_data.half_extent[face.normal_index] * face.normal_sign;

            if (edit_data.hit.miss() && glm::distance(local_hit_position[face.normal_index], extent_axis) < std::numeric_limits<float>::epsilon() * 10.0f) {

                edit_data.hit.entity = selected_entity;
                edit_data.hit.distance = far;
                edit_data.half_extent = edit_data.half_extent;

                const glm::vec3 position = (mouse_ray.origin + mouse_ray.dir * edit_data.hit.distance) - (face.world_normal * VOXEL_SIZE_HALF);
                const glm::vec3 local_position = glm::inverse(transform.get_world_matrix()) * glm::vec4 {position, 1.0f};

                edit_data.hit.coord = (local_position + edit_data.half_extent) * static_cast<float>(VOXELS_PER_UNIT);

                draw_selected_face(face, edit_data.half_extent, transform);
            }
        }
    }
    if (edit_data.hit.miss() || edit_data.hit.entity != selected_entity) return;

    draw_selection(edit_data, transform);

    if (engine.input.is_mouse_button_pressed(MouseButton::LEFT)) {
        const MaterialIndex material_index = editor.windows[Editor::Mode::EDIT].get<Palette>().get_selected_material_index();

        renderer->resource->blas->set_voxel(edit_data.hit.coord.x, edit_data.hit.coord.y, edit_data.hit.coord.z, material_index);
        renderer->resource->update();
    }
}

}  // namespace tmt