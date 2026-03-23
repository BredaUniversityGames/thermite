#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "../../../editor/all.hpp"

struct Missile;

namespace game {

class SmallEnemy : public tmt::GameComponent<SmallEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Small Enemy"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}
    tmt::Entity core = entt::null;
};

}  // namespace game

TMT_OBJECT(game::SmallEnemy, (core));
