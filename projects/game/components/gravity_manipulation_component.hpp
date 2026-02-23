#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

namespace game {

class GravityManipulationComponent : public tmt::GameComponent<GravityManipulationComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Gravity Manipulation Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

   private:
};

}  // namespace game
TMT_OBJECT(game::GravityManipulationComponent, ());