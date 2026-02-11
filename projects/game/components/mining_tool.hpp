#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/systems/core/resources/stencil.hpp"

class MiningTool : public tmt::GameComponent<Wallet> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Tool"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    private:
    };
TMT_OBJECT(MiningTool, ());