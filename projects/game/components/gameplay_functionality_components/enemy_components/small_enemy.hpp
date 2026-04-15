#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "../../../editor/all.hpp"

namespace game {

class SmallEnemy : public tmt::GameComponent<SmallEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Small Enemy"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    void die();

    tmt::Entity thermite = entt::null;
    // tmt::Entity core = entt::null;
    tmt::Entity explosion = entt::null;
};

}  // namespace game

TMT_OBJECT(game::SmallEnemy, (thermite, explosion/*, core*/));
