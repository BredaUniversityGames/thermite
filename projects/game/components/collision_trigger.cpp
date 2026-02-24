#include "collision_trigger.hpp"

#include "events.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/physics/physics_system.hpp"

void game::CollisionTrigger::fixed_update(const tmt::FrameData& time) {
    // TODO update aabb in case this moves
    const auto& bvh = tmt::engine.ecs.systems.get<tmt::Physics>().get_bvh();
    // TODO mask it
    auto overlap_hits = bvh.overlap(get_aabb());
    if (!overlap_hits.empty()) {
        auto group = tmt::engine.ecs.get_registry().group<tmt::VoxelBody>(entt::get<tmt::Transform>);
        for (uint32_t hit : overlap_hits) {
            tmt::Entity other_entity = group[hit];
            // do stuff with the entity
            // TODO for now just kill it
            tmt::engine.ecs.get_dispatcher().trigger(TriggerCollisionEvent { .trigger = entity, .other_object = other_entity });
        }
    }
}
void game::CollisionTrigger::start() {
    /* constexpr glm::vec3 BIG_CONST_VEC3 { BIG_F32 };
     if (glm::all(glm::equal(collision_shape.aabb.max_bounds, -BIG_CONST_VEC3)) || glm::all(glm::equal(collision_shape.aabb.min_bounds, BIG_CONST_VEC3))) {
         tmt::Log::warn("Defaulting collision trigger entity {} ", entity);
         collision_shape.calculate_aabb(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position());
     }      */

    // tmt::engine.ecs.get_registry().on_update<tmt::Transform>().connect<&CollisionTrigger::on_transform_updated>(this);
}

tmt::Aabb game::CollisionTrigger::get_aabb() const {
    return tmt::Aabb { collision_shape.aabb.min_bounds, collision_shape.aabb.max_bounds };
}
void game::CollisionTrigger::on_transform_updated(entt::registry& reg, tmt::Entity e) {
    if (e != entity) {
        return;
    }
    collision_shape.calculate_aabb(reg.get<tmt::Transform>(e).get_world_position());
}

void game::CollisionTrigger::end() {
    // tmt::engine.ecs.get_registry().on_update<tmt::Transform>().disconnect<&CollisionTrigger::on_transform_updated>(this);
}
void game::CollisionTrigger::draw_debug_lines() const {
    tmt::engine.polyline.use_color(color);
    tmt::engine.polyline.use_line_width(line_width);

    collision_shape.calculate_aabb(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position());
    auto min_aabb = collision_shape.aabb.min_bounds;
    auto max_aabb = collision_shape.aabb.max_bounds;
    tmt::engine.polyline.draw_aabb(min_aabb, max_aabb);
}
