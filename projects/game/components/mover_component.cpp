#include "mover_component.hpp"

#include "events.hpp"
#include "engine/core/polyline.hpp"
void game::MoverComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().disconnect<&MoverComponent::update_movement>(this);
}
void game::MoverComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerMovementEvent>().disconnect<&MoverComponent::update_movement>(this);
}
void game::MoverComponent::draw_debug_lines() const {
    cfg.set_values();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    tmt::engine.polyline.draw_arrow(transform.get_world_position(), transform.get_forward(), movement_speed);
}

void game::MoverComponent::update_movement(const TriggerMovementEvent& event) {
    if (event.trigger != trigger_entity) {
        return;
    }
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto delta = transform.get_forward() * movement_speed * tmt::engine.frame_data().delta_time;
    transform.translate(delta);

    tmt::engine.ecs.get_dispatcher().trigger<MovementUpdateEvent>({ .moved_entity = entity, .delta = delta });
}
