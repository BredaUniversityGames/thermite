#pragma once
#include "engine/systems/gameplay/game_component.hpp"

#include "events.hpp"
namespace game {

class OreCollector : public tmt::GameComponent<OreCollector> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OreCollector"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    // Shoots rays
    float radius = 1.0f;
    // debugging settings
    glm::vec4 color { 1.0f };
    float line_width = 1.0f;

   private:
    uint64_t ore_count = 0u;
    void on_collision_trigger(const TriggerCollisionEvent& trigger);
};

}  // namespace game
TMT_OBJECT(game::OreCollector, (radius, color, line_width));
