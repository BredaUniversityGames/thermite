#include "physics_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/components.hpp"

#include "tools/fmt_defines.hpp"

namespace tmt {

void Physics::on_start() {
    Log::info("Physics on_start");
    /* ECS testing */
    {
        Entity entity = engine.ecs.create_entity();
        Log::info("Created entity with ID: {}", entity);

        auto& name_comp = engine.ecs.add_component<Name>(entity);
        name_comp.name = "Player";

        Log::info("Added Name component: {}", name_comp);

        auto& transform = engine.ecs.add_component<Transform>(entity);
        transform.position = {0.0f, 1.0f, 2.0f};
        transform.rotation = {0.0f, 90.0f, 0.0f};
        transform.scale = {1.0f, 1.0f, 1.0f};

        Log::info("Added Transform component: {}", transform);

        auto same_name = engine.ecs.get_component<Name>(entity);
        Log::info("Retrieved Name component: {}", same_name);

        engine.ecs.remove_component<Name>(entity);
        Log::info("Removed Name component from entity ID: {}", entity);

        auto* try_name = engine.ecs.try_get_component<Name>(entity);
        if (try_name == nullptr) {
            Log::info("Name component correctly not found after removal.");
        } else {
            Log::warn(
                "Name component was found when it should have been removed!"
            );
        }

        engine.ecs.destroy_entity(entity);
    }
    {
        Entity entity = engine.ecs.create_entity();
        Log::info("Created entity with ID: {}", entity);

        auto comps = engine.ecs.add_component<Name, Transform>(entity);
        std::get<Name>(comps).name = "Enemy";
        std::get<Transform>(comps).position = {5.0f, 0.0f, -3.0f};
        Log::info(
            "Added multiple components: {}, {}", std::get<Name>(comps),
            std::get<Transform>(comps)
        );

        auto [name_comp, transform_comp] =
            engine.ecs.get_component<Name, Transform>(entity);
        Log::info(
            "Retrieved multiple components: {}, {}", name_comp, transform_comp
        );

        engine.ecs.remove_component<Name, Transform>(entity);
        Log::info("Removed multiple components from entity ID: {}", entity);

        auto [try_name, try_transform] =
            engine.ecs.try_get_component<Name, Transform>(entity);
        if (try_name == nullptr && try_transform == nullptr) {
            Log::info(
                "Both Name and Transform components correctly not found after "
                "removal."
            );
        } else {
            Log::warn(
                "One or both components were found when they should have been "
                "removed!"
            );
        }

        engine.ecs.destroy_entity(entity);
    }
}

void Physics::on_update(const FrameData& time) { Log::info("Physics on_update with delta time: {}", time.delta_time); }

void Physics::on_fixed_update(const FrameData& time) { Log::info("Physics on_fixed_update with delta time: {}", Engine::Config::fixed_time_step); }

void Physics::on_end() { Log::info("Physics on_end"); }

}  // namespace tmt
