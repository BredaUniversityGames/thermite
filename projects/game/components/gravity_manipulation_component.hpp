#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

namespace game {

class GravityManipulationComponent : public tmt::GameComponent<GravityManipulationComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Gravity Manipulation Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float range = 5.0f;
    float forward_offset = 2.0f;
    float vertical_offset = -0.2f;
    float max_mass = 200.0f;
    float pull_strength = 20.0f;
    float push_strength = 40.0f;

   private:
    std::vector<entt::entity> currently_manipulated_entities;
    void grav_point_check();
    void grav_attract();
    void grav_shoot();
    void calculate_attract_point();
    glm::vec3 attract_point;
};

}  // namespace game
TMT_OBJECT(game::GravityManipulationComponent, (range, forward_offset, vertical_offset, max_mass, pull_strength, push_strength));