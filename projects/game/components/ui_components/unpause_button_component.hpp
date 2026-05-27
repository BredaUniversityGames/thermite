#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class UnpauseButtonComponent : public tmt::GameComponent<UnpauseButtonComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Unpause Button Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void unpause(tmt::Button::Context context);

   private:
    tmt::Entity menu_controller_entity = entt::null;
    
};

}  // namespace game

TMT_GAME_COMPONENT(game::UnpauseButtonComponent);
