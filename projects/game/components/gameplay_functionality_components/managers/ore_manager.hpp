#pragma once
#include "engine/shared/ray.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

namespace game {

class OreManager : public tmt::GameComponent<OreManager> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Ore Manager"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void trigger_thermite_explosion(tmt::Entity voxel_entity);

    float thermite_ore_explosion_radius = 1.0f;
    float thermite_ore_explosion_damage = 10.0f;

   private:

};

}  // namespace game
TMT_OBJECT(game::OreManager, ());
