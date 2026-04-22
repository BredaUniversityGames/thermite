#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

class ResourceLimitChecker : public tmt::GameComponent<ResourceLimitChecker> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Resource Limit Checker (Button)"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity wallet_entity = entt::null;

    float check_interval = 1.0f;

   private:
    bool resource_check();
    void button_click(tmt::Button::Context context);
    float interval_counter = 0.0f;
};

}  // namespace game

TMT_OBJECT(game::ResourceLimitChecker, (wallet_entity));
