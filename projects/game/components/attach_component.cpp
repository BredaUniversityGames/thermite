#include "attach_component.hpp"

#include "engine/core/polyline.hpp"
#include "glm/gtx/norm.inl"

void game::AttachComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<AttachAttemptEvent>().connect<&AttachComponent::on_check_range_to_attach>(this);
}
void game::AttachComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<AttachAttemptEvent>().disconnect<&AttachComponent::on_check_range_to_attach>(this);
}
void game::AttachComponent::draw_debug_lines() const {
    cfg.set_values();

    if (on_check_range_to_attach({ entity_that_attaches })) {
        tmt::engine.polyline.use_color(glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f });
    }
    tmt::engine.polyline.draw_sphere(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position(), radius);
}
bool game::AttachComponent::on_check_range_to_attach(const AttachAttemptEvent& event) const {
    if (event.entity == entt::null || tmt::engine.ecs.valid(event.entity) == false) {
        return false;
    }
    auto entity_pos = tmt::engine.ecs.get_component<tmt::Transform>(event.entity).get_world_position();
    const auto pos = tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position();

    return glm::distance2(entity_pos, pos) < radius * radius;
}
