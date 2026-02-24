#include "ore_collector.hpp"

#include "events.hpp"
#include "collision_trigger.hpp"
#include "engine/core/polyline.hpp"
void game::OreCollector::start() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().connect<&OreCollector::on_collision_trigger>(this);
}
void game::OreCollector::update(const tmt::FrameData& time) {}

void game::OreCollector::end() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().disconnect<&OreCollector::on_collision_trigger>(this);
}
void game::OreCollector::draw_debug_lines() const {
    tmt::engine.polyline.use_color(color);
    tmt::engine.polyline.use_line_width(line_width);

    auto* collisionTrigger = tmt::engine.ecs.try_get_component<CollisionTrigger>(entity);
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    if (collisionTrigger == nullptr) {
        return;
    }
    const auto& shape = collisionTrigger->collision_shape.calculate_aabb(transform.get_world_position());

    auto min_aabb = shape.min_bounds;
    auto max_aabb = shape.max_bounds;
    tmt::engine.polyline.draw_aabb(min_aabb, max_aabb);
}

void game::OreCollector::on_collision_trigger(const TriggerCollisionEvent& trigger) {
    // TODO do something smarter maybe, check type of ore etc.
    if (trigger.trigger != entity) {
        return;
    }
    if (!tmt::engine.ecs.get_registry().valid(trigger.other_object)) {
        return;
    }
    tmt::engine.ecs.destroy_entity(trigger.other_object);

    tmt::Log::info("Ore count is {}", ++ore_count);
}
