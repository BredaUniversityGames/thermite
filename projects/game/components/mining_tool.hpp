#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

class MiningTool : public tmt::GameComponent<MiningTool> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Tool"; }

    void start() override {}
    void update(const tmt::FrameData& time) override;
    void end() override {}

    void mine();
    tmt::ResourceRef<tmt::Stencil> stencil; //currently active stencil
    float mining_cooldown;

   private:
    std::vector<tmt::ResourceRef<tmt::Stencil>> stencils;
    float mining_time_since_last_mine_action;
    };
TMT_OBJECT(MiningTool, (stencil, mining_cooldown));