#pragma once
#include "ore_properties.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Wallet : public tmt::GameComponent<Wallet> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Wallet"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    // Currencies
    float dollars = 0.0f;

    std::array<uint64_t, magic_enum::enum_count<OreProperties::OreResources>()> resource_counts {};
};

}  // namespace game
TMT_OBJECT(game::Wallet, (dollars, resource_counts));
