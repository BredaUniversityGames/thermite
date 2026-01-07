#pragma once
#include "engine/systems/gameplay/game_component.hpp"

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float speed = 1.0f;
};
TMT_OBJECT(Player, (speed));