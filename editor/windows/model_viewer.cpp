#include "model_viewer.hpp"

#include "editor.hpp"
#include "viewport.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"

namespace tmt {

namespace {

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

void draw_face(int32_t face_index, const Ray& ray, const Transform& transform, const glm::vec3& half_extent) {
    const auto& result = std::div(face_index, 2);

    glm::vec3 face_normal {};
    const float sign = (result.rem == 0 ? 1.0f : -1.0f);
    face_normal[result.quot] = sign;

    const glm::vec3 world_normal = transform.get_world_matrix() * glm::vec4 {face_normal, 0.0f};
    const glm::vec3 face_center = transform.get_world_matrix() * glm::vec4 {face_normal * half_extent, 1.0f};

    const glm::vec3 camera_position = engine.renderer.get_debug_transform().get_world_position();

    if (glm::dot(world_normal, camera_position - face_center) < 0.0f) {
        glm::vec3 other = half_extent;
        other[result.quot] = 0.0f;

        engine.polyline.use_line_width(2.0f, true);
        engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 1.0f});
        engine.polyline.draw_obb(face_center, other, transform.get_world_rotation());
    }

    if (glm::dot(world_normal, ray.dir) < 0.0f) return;

    glm::vec3 contact;
    if (!plane_intersect(contact, ray, world_normal, face_center)) return;

    glm::vec3 min = -half_extent;
    min[result.quot] = half_extent[result.quot] * sign;
    min = transform.get_world_matrix() * glm::vec4 {min, 1.0f};

    glm::vec3 max = half_extent;
    max[result.quot] = half_extent[result.quot] * sign;
    max = transform.get_world_matrix() * glm::vec4 {max, 1.0f};

    glm::vec3 distance = contact - face_center;
    const bool contains = glm::abs(glm::dot(distance, transform.get_right())) <= half_extent.x && glm::abs(glm::dot(distance, transform.get_up())) <= half_extent.y &&
                          glm::abs(glm::dot(distance, transform.get_forward())) <= half_extent.z;


    if (!contains) return;

    engine.polyline.use_line_width(4.0f, false);
    engine.polyline.use_color(glm::vec4 {0.8f, 0.8f, 0.8f, 0.4f});
    switch (result.quot) {
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

}  // namespace

void ModelViewer::display() {
    const ImVec2 content_start {0.0f, ImGui::GetFrameHeight()};
    ImGui::SetCursorPos(content_start);

    const ImVec2 size = ImGui::GetWindowSize() - content_start;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    ImGui::Image(engine.renderer.render_view.imgui_viewport, size);

    Viewport& viewport = editor.windows[Editor::Mode::GAME].get<Viewport>();

    viewport.is_hovered = ImGui::IsItemHovered();
    const ImVec2 mouse_pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
    mouse_position.x = viewport.mouse_pos.x = mouse_pos.x - content_start.x;
    mouse_position.y = viewport.mouse_pos.y = mouse_pos.y - content_start.y;
}

void ModelViewer::on_editor_update(const FrameData& time) {
    editor.windows[Editor::Mode::GAME].get<Viewport>().update_debug_camera(time);

    const entt::basic_group group = engine.ecs.get_registry().group<const VoxelRenderer>(entt::get<Transform>);
    for (auto&& [entity, renderer, transform] : group.each()) {
        const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);
        const glm::vec3 half_extent = glm::vec3 {renderer.resource->size} / static_cast<float>(2 * VOXELS_PER_UNIT);

        for (int32_t i = 0; i < 6; i++) {
            draw_face(i, mouse_ray, transform, half_extent);
        }
    }
}

}  // namespace tmt