#pragma once
#include "engine/core/system.hpp"
#include "engine/events/input.hpp"
#include "engine/core/entity.hpp"

namespace tmt {

class ButtonManager : public ISystem {
   public:
    std::string get_name() override { return "ButtonManager"; }
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void update_states(const tmt::FrameData& time);
    void on_end() override;

    void select_button(const Entity button);

   private:
    float hold_time = 0.0f;
    Entity selected_button = entt::null;
};

}  // namespace tmt