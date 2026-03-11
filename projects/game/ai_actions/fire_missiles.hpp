#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class FireMissiles : public tmt::GoapAction {
   public:
    FireMissiles() {
        preconditions["in_aggro_range"] = true;
        preconditions["missiles_ready"] = true;
        effects["hit_player"] = true;
        cost = 5.f;
    }

    tmt::Entity player = entt::null;
    
    int missiles = 0;
    float burst_interval = 0.2f;
    float interval_timer = 0.0f;

    std::string get_id() const override { return "FireMissiles"; }

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
};
