#pragma once
#include "engine/systems/gameplay/game_component.hpp"

#include "projects/game/data_headers/events.hpp"
namespace game {

class OreCollector : public tmt::GameComponent<OreCollector> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OreCollector"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    // Shoots rays
    float radius = 1.0f;
    // debugging settings
    tmt::Entity wallet_entity { entt::null };
    float penalty_percentage = 0.6f;

   private:
    bool penalty { true };
    uint64_t ore_count = 0u;
    void on_collision_trigger(const TriggerCollisionEvent& trigger);
    void on_end_run(const EndRun& end_run);
    void transfer_money();
};

}  // namespace game
TMT_OBJECT(game::OreCollector, (radius, wallet_entity, penalty_percentage));
