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

namespace tmt {

namespace {

// Plane intersection code from: https://stackoverflow.com/questions/7168484/3d-line-segment-and-plane-intersection
bool plane_intersect(glm::vec3& contact, const Ray& ray, const glm::vec3& normal, const glm::vec3& coord) {
    // get d value
    const float d = glm::dot(normal, coord);

    if (glm::dot(normal, ray.dir) == 0.0f) {
        return false;  // No intersection, the line is parallel to the plane
    }

    // Compute the X value for the directed line ray intersecting the plane
    const float x = (d - glm::dot(normal, ray.origin)) / glm::dot(normal, ray.dir);

    // output contact point
    contact = ray.origin + ray.dir * x;  // Make sure your ray vector is normalized
    return true;
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

bool contains_axis(const glm::vec3& distance, const glm::vec3& direction, const float half_extent) { return glm::abs(glm::dot(distance, direction)) <= half_extent; }

struct FaceData {
    float normal_sign {0.0f};
    int32_t normal_index {-1};

    glm::vec3 world_normal {};
    glm::vec3 face_center {};
    glm::vec3 half_extent {};

    Entity entity {entt::null};
};

FaceData calculate_face_info(const int32_t face_index, const Transform& transform, const glm::vec3& half_extent) {
    const std::div_t& result = std::div(face_index, 2);

    glm::vec3 face_normal {};
    const float sign = (result.rem == 0 ? 1.0f : -1.0f);
    face_normal[result.quot] = sign;

    FaceData info {.normal_sign = sign, .normal_index = result.quot, .half_extent = half_extent};
    info.world_normal = transform.get_world_matrix() * glm::vec4 {face_normal, 0.0f};
    info.face_center = transform.get_world_matrix() * glm::vec4 {face_normal * half_extent, 1.0f};

    return info;
}

bool draw_face(const FaceData& info, const Transform& transform) {
    const glm::vec3 camera_position = engine.renderer.get_debug_transform().get_world_position();
    if (glm::dot(info.world_normal, camera_position - info.face_center) >= 0.0f) return false;

    glm::vec3 box_half_extent = info.half_extent;
    box_half_extent[info.normal_index] = 0.0f;

    engine.polyline.use_line_width(2.0f, true);
    engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 1.0f});
    engine.polyline.draw_obb(info.face_center, box_half_extent, transform.get_world_rotation());

    return true;
}

void draw_selected_face(const FaceData& info, const Transform& transform) {
    glm::vec3 min = -info.half_extent;
    min[info.normal_index] = info.half_extent[info.normal_index] * info.normal_sign;
    min = transform.get_world_matrix() * glm::vec4 {min, 1.0f};

    engine.polyline.use_line_width(4.0f, false);
    engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 0.4f});
    switch (info.normal_index) {
        case 0:
            draw_face_grid(min, transform.get_forward(), transform.get_up(), info.half_extent[2] * 2.0f, info.half_extent[1] * 2.0f);
            break;

        case 1:
            draw_face_grid(min, transform.get_right(), transform.get_forward(), info.half_extent[0] * 2.0f, info.half_extent[2] * 2.0f);
            break;

        case 2:
            draw_face_grid(min, transform.get_right(), transform.get_up(), info.half_extent[0] * 2.0f, info.half_extent[1] * 2.0f);
            break;

        default:
            break;
    }
}

void draw_selection(const glm::vec3& half_extent, const Transform& transform, const Hit& hit) {
    engine.polyline.use_line_width(2.0f);
    engine.polyline.use_color(glm::vec4 {1.0f, 0.5f, 0.5f, 1.0f});

    const glm::vec3 local_voxel_pos = glm::vec3 {hit.coord} / static_cast<float>(VOXELS_PER_UNIT) - half_extent + VOXEL_SIZE_HALF;
    const glm::vec3 world_voxel_pos = transform.get_world_matrix() * glm::vec4 {local_voxel_pos, 1.0f};

    engine.polyline.draw_obb(world_voxel_pos, glm::vec3 {VOXEL_SIZE_HALF}, transform.get_world_rotation());
}

std::atomic_bool test{true};

}  // namespace

void ModelViewer::display() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Load"))
            open_file_dialog(
                [this](const IO::FileLocation& location) {
                    test.store(false);

                    const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
                    const ResourceRef<VoxelVolume> copy = engine.resources.copy_resource<VoxelVolume>(resource);
                    editing_volume = copy;

                    test.store(true);
                    test.notify_one();
                },
                {{"Voxel file", "svh;vengi"}}
            );

        ImGui::EndMenuBar();
    }
    test.wait(false);

    if (editing_volume && editing_entity == entt::null) create_editing_entity();

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
    Hit hit = engine.renderer.trace_ray(mouse_ray);

    FaceData selected_face {};
    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);
    for (auto&& [entity, renderer, transform] : group.each()) {
        const glm::vec3 half_extent = glm::vec3 {renderer.resource->size} / static_cast<float>(2 * VOXELS_PER_UNIT);

        for (int32_t i = 0; i < 6; i++) {
            FaceData face = calculate_face_info(i, transform, half_extent);

            if (!draw_face(face, transform)) continue;

            // Check if the ray hits the plane which the face is on.
            glm::vec3 contact;
            if (!plane_intersect(contact, mouse_ray, face.world_normal, face.face_center)) continue;

            // Check if the ray hit the plane is in front of the already hit object.
            const float contact_distance = glm::distance(mouse_ray.origin, contact);
            if (contact_distance > hit.distance) continue;

            // Check if we hit the plane inside the face area.
            const glm::vec3 distance = contact - face.face_center;
            const bool contains = contains_axis(distance, transform.get_right(), face.half_extent.x) && contains_axis(distance, transform.get_up(), face.half_extent.y) &&
                                  contains_axis(distance, transform.get_forward(), face.half_extent.z);

            if (!contains) continue;

            selected_face = face;
            selected_face.entity = entity;
            hit.distance = contact_distance;
        }
    }

    if (hit.entity != entt::null) {
        const auto& [renderer, transform] = engine.ecs.get_component<VoxelRenderer, Transform>(hit.entity);
        const glm::vec3 half_extent = glm::vec3 {renderer.resource->size} / static_cast<float>(2 * VOXELS_PER_UNIT);

        draw_selection(half_extent, transform, hit);
        return;
    }

    if (selected_face.entity == entt::null) return;

    const Transform& transform = engine.ecs.get_component<Transform>(selected_face.entity);
    draw_selected_face(selected_face, transform);

    const glm::vec3 position = (mouse_ray.origin + mouse_ray.dir * hit.distance) + (-selected_face.world_normal * VOXEL_SIZE_HALF);
    const glm::vec3 local_position = glm::inverse(transform.get_world_matrix()) * glm::vec4 {position, 1.0f};

    hit.coord = (local_position + selected_face.half_extent) * static_cast<float>(VOXELS_PER_UNIT);
    draw_selection(selected_face.half_extent, transform, hit);
}

void ModelViewer::on_editor_end() {
    editing_volume = {};
}

void ModelViewer::create_editing_entity() {
    editing_entity = engine.ecs.create_entity();
    engine.ecs.add_component<VoxelRenderer>(editing_entity).resource = editing_volume;
}

}  // namespace tmt