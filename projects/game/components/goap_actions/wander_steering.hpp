#pragma once

#include "engine/systems/ai/goap/components/goap_action.hpp"

namespace tmt {

class WanderSteering : public GoapAction {
   public:
    WanderSteering() {
        preconditions["at_target"] = true;
        effects["can_move"] = true;
        effects["at_target"] = false;
        cost = 1.f;
        wander_duration = 5.f;
    }

    std::string get_id() const override { return "a_WanderSteering"; }

    void on_start(Entity agent) override;
    void on_tick(Entity /*agent*/, float dt) override;
    bool is_done(Entity /*agent*/) const override;
    void on_finished(Entity agent) override;
    void on_interrupt(Entity agent) override;

   private:
    float wander_duration;
    float time_wandering = 0.f;
};

}  // namespace tmt
