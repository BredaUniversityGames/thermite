#pragma once
#include "engine/core/ecs.hpp"
#include "world_state.hpp"

namespace tmt {

class GoapAction {
   public:
    GoapAction() = default;
    virtual ~GoapAction() = default;

    // These are used by the planner
    std::unordered_map<std::string, bool> preconditions;
    std::unordered_map<std::string, bool> effects;
    float cost = 1.f;

    // Used during action execution
    bool is_running = false;

    // readable name
    virtual const char* get_name() const = 0;

    // Planner helper functions
    virtual bool check_preconditions(const WorldState& ws) const;
    virtual void apply_effects(WorldState& ws) const;  // calls apply() from ws

    // action execuion helpers
    virtual void on_start(entt::entity agent, entt::registry& ecs) {}         // Called once when the action begins
    virtual void on_tick(Entity agent, Registry& ecs, float dt) {}            // Called every fram while action is running
    virtual void on_finished(Entity agent, Registry& ecs) {}                  // Called only when the action finishes normally
    virtual bool is_done(Entity agent, Registry& ecs) const { return true; }  // Called by the GOAP system to check if the action is done

    // Interrupt callback
    virtual void on_interrupt(Entity agent, Registry& ecs) {}  // Called when the action is cancelled before completion
};

}  // namespace tmt
