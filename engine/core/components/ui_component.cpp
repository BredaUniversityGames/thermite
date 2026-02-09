#include "ui_component.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/window.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

bool contains(const glm::vec2 size, const glm::vec2 point, const glm::vec2 center, const glm::vec2 scale, const glm::vec3 rotation, const glm::vec2 pivot) {
    const glm::vec2 scaled_size = size * abs(scale);

    if (scaled_size.x <= 0.0f || scaled_size.y <= 0.0f) return false;

    const glm::vec2 scaled_half_size = scaled_size * 0.5f;

    const glm::vec3 cos(glm::cos(rotation));
    const glm::vec3 sin(glm::sin(rotation));

    glm::vec2 right(cos.y * cos.z, cos.y * sin.z);
    glm::vec2 up(sin.x * sin.y * cos.z - cos.x * sin.z, sin.x * sin.y * sin.z + cos.x * cos.z);

    if (scale.x <= 0.0f) right = -right;
    if (scale.y <= 0.0f) up = -up;

    // Rotation stuff written by ClaudeAI
    const float det = right.x * up.y - right.y * up.x;
    if (std::abs(det) < 1e-6f) return false;
    const float inv_det = 1.0f / det;

    const glm::vec2 local_pivot_offset = scaled_size * (pivot - glm::vec2(0.5f));
    const glm::vec2 rotated_pivot_offset = right * local_pivot_offset.x + up * local_pivot_offset.y;
    const glm::vec2 rect_center = center - rotated_pivot_offset;

    const glm::vec2 offset(point - rect_center);
    const glm::vec2 local((up.y * offset.x - up.x * offset.y) * inv_det, (-right.y * offset.x + right.x * offset.y) * inv_det);

    const bool between_x = local.x >= -scaled_half_size.x && local.x <= scaled_half_size.x;
    const bool between_y = local.y >= -scaled_half_size.y && local.y <= scaled_half_size.y;
    return between_x && between_y;
}

glm::vec2 AnchorHelper::calculate_anchor_offset_in_rect(const glm::vec2 size, const Anchor anchor) {
    switch (anchor) {
        case Anchor::TopLeft:
            return glm::vec2(0.0f, 0.0f);
        case Anchor::TopCenter:
            return glm::vec2(size.x / 2.0f, 0.0f);
        case Anchor::TopRight:
            return glm::vec2(size.x, 0.0f);
        case Anchor::MiddleLeft:
            return glm::vec2(0.0f, size.y / 2.0f);
        case Anchor::MiddleCenter:
            return glm::vec2(size.x / 2.0f, size.y / 2.0f);
        case Anchor::MiddleRight:
            return glm::vec2(size.x, size.y / 2.0f);
        case Anchor::BottomLeft:
            return glm::vec2(0.0f, size.y);
        case Anchor::BottomCenter:
            return glm::vec2(size.x / 2.0f, size.y);
        case Anchor::BottomRight:
            return glm::vec2(size.x, size.y);
        default:
            return glm::vec2(0.0f);
    }
}

glm::vec2 AnchorHelper::calculate_anchor_offset(const Entity entity) {
    auto* transform = engine.ecs.get_registry().try_get<Transform>(entity);
    auto* ui = engine.ecs.get_registry().try_get<UIComponent>(entity);
    if (!transform || !ui) return glm::vec2(0.0f);

    if (!transform->has_parent()) {
        glm::vec2 offset = calculate_anchor_offset_in_rect(engine.renderer.render_view.gpu_view.resolution, ui->anchor);
        return offset;
    }

    auto parentEntity = transform->get_parent();
    auto* parentUI = engine.ecs.get_registry().try_get<UIComponent>(parentEntity);
    auto* parentTransform = engine.ecs.get_registry().try_get<Transform>(parentEntity);
    if (!parentUI || !parentTransform) {
        glm::vec2 offset = calculate_anchor_offset_in_rect(engine.renderer.render_view.gpu_view.resolution, ui->anchor);
        return offset;
    }

    glm::vec2 localOffset = calculate_anchor_offset_in_rect(parentUI->size, ui->anchor) - parentUI->size * parentUI->pivot;

    glm::mat4 parentRS = parentTransform->get_world_matrix();
    parentRS[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 transformed = parentRS * glm::vec4(localOffset, 0.0f, 1.0f);
    glm::vec2 rsOffset(transformed.x, transformed.y);

    return rsOffset + calculate_anchor_offset(parentEntity);
}

bool AnchorHelper::is_inside(const Entity entity, const glm::vec2& viewport_point) {
    if (entity == entt::null) return false;

    const auto* ui_component = engine.ecs.get_registry().try_get<UIComponent>(entity);
    if (!ui_component) return false;

    auto* transform = engine.ecs.get_registry().try_get<Transform>(entity);
    if (!transform) return false;

    // get ui elements
    const auto rect = ui_component->size;
    const auto offset = calculate_anchor_offset(entity);

    // apply transform values
    const glm::vec2 world_pos = transform->get_world_position();
    const glm::vec2 world_scale = transform->get_world_scale();
    const glm::vec3 world_rotation = glm::eulerAngles(transform->get_world_rotation());

    return contains(rect, viewport_point, world_pos + offset, world_scale, world_rotation, ui_component->pivot);
}
}  // namespace tmt