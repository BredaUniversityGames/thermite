#pragma once
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

class FireLaser : public tmt::GoapAction {
   public:
    FireLaser() {
        preconditions["in_aggro_range"] = true;
        preconditions["laser_ready"] = true;
        effects["hit_player"] = true;
        cost = 2.f;
    }

    enum Mode { LINEAR, EXPONENTIAL } mode;

    tmt::Entity player = entt::null;

    float duration = 0.0f;
    float accuracy = 0.0f;
    glm::vec3 direction = glm::vec3(0, 0, 0);
    glm::vec3 target_pos = glm::vec3(0, 0, 0);
    float prediction_length = 0.0f;
    glm::vec3 last_player_pos = glm::vec3(0, 0, 0);
    glm::vec3 last_to_player_dir = glm::vec3(0, 0, 0);

    std::string get_id() const override { return "FireLaser"; }

    void on_start(tmt::Entity) override;
    void on_tick(tmt::Entity agent, float dt) override;
    bool is_done(tmt::Entity agent) const override;
    void on_finished(tmt::Entity) override {}
};
