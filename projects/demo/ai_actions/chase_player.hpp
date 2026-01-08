#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class ChasePlayer : public tmt::GoapAction {
   public:
    ChasePlayer() {
        preconditions["player_visible"] = true;
        effects["player_in_range"] = true;
        cost = 4.f;
    }
    std::string get_id() const override { return "ChasePlayer"; };

    tmt::Entity player = entt::null;

    bool done_walking = false;

    void on_start(tmt::Entity, tmt::Registry&) override;

    void on_tick(tmt::Entity /* agent */, tmt::Registry& /* ecs */, float dt) override;

    bool is_done(tmt::Entity /* agent */, tmt::Registry& /* ecs */) const override;

    void on_finished(tmt::Entity /* agent */, tmt::Registry& /* ecs */) override {}

    void on_interrupt(tmt::Entity /* agent */, tmt::Registry& /* ecs */) override {}
};
