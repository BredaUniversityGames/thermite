#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

struct UpgradeParent : tmt::GameComponent<UpgradeParent> {
    using GameComponent::GameComponent;
    void start() override {};
    void update(const tmt::FrameData& time) override {};
    void end() override {};
    static std::string_view get_name() { return "UpgradeParent"; }
};

}  // namespace game
TMT_OBJECT_INSPECT_EMPTY(game::UpgradeParent);
TMT_OBJECT_SERIALIZE_EMPTY(game::UpgradeParent);
