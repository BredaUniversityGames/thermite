#include "goap_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

namespace tmt {

void Goap::on_start() { Log::info("Goap on_start"); }

void Goap::on_update(const FrameData& time) 
{
    auto& registry = tmt::engine.ecs.get_registry();
    float dt = time.delta_time;
    
    for (auto [entity, goap, goal, world]
        : registry.view<GoapComponent, GoalComponent, world_state>().each()) 
    {
        process_agent(registry, entity, dt);
    }
}

void Goap::on_fixed_update(const FrameData&) {}

void Goap::on_end() { Log::info("Goap on_end"); }


// ------------------------------------------------------
// Main per-agent update
// ------------------------------------------------------
void Goap::process_agent(tmt::Registry& ecs, tmt::Entity entity, float dt) 
{
    auto& goap = ecs.get<GoapComponent>(entity);
    auto& goal = ecs.get<GoalComponent>(entity);
    auto& ws = ecs.get<world_state>(entity);

    update_goal(ecs, entity, goap, goal, ws);
    if (goap.needs_replan) update_plan(ecs, entity, goap, goal, ws);
    update_action(ecs, entity, goap, dt);
}

// ------------------------------------------------------
// Step 1: goal assignment
// ------------------------------------------------------
void Goap::update_goal(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, GoalComponent& goal, world_state& ws) 
{
    if (goap.has_goal) return;

    // TODO: goal selection logic here (look at world_state)

    goap.has_goal = true;
    goap.needs_replan = true;
}

// ------------------------------------------------------
// Step 2: planning
// ------------------------------------------------------
void Goap::update_plan(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, GoalComponent& goal, world_state& ws) 
{
    goap.clear_plan();

    // TODO: implement A* planning using goap.actions

    if (!goap.plan.empty()) {
        goap.current_index = 0;
        goap.needs_replan = false;
    } else {
        // Could not create a valid plan
        Log::warn("GOAP: planning failed for agent {}", int(entity));
    }
}

// ------------------------------------------------------
// Step 3: action execution
// ------------------------------------------------------
void Goap::update_action(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, float dt) 
{
    GoapAction* action = goap.get_current_action();
    if (!action) return;  // no plan yet

    // TODO: logic for running the action
}

}  // namespace tmt
