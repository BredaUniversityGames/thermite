#pragma once
#include "core/system.hpp"
#include "components/goal_component.hpp"
#include "components/world_state.hpp"
#include "components/goap_component.hpp"


namespace tmt {

class Goap : public ISystem {

   public:

    Goap() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Goap System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    
private:
    void process_agent(tmt::Registry& ecs, tmt::Entity entity, float dt);

    // Step 1: Assign goal if needed
    void update_goal(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, GoalComponent& goal, world_state& ws);

    // Step 2: Build plan if needed
    void update_plan(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, GoalComponent& goal, world_state& ws);

    // Step 3: Execute current action
    void update_action(tmt::Registry& ecs, tmt::Entity entity, GoapComponent& goap, float dt);
};

}  // namespace tmt
