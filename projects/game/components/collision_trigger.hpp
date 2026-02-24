#pragma once
#include "collision_shapes.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/shared/aabb.hpp"
namespace game {

class CollisionTrigger : public tmt::GameComponent<CollisionTrigger> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "CollisionTrigger"; }

    void start() override;
    void update(const tmt::FrameData& time) override {}
    void fixed_update(const tmt::FrameData& time) override;
    void end() override;

    AABBShape collision_shape;
    tmt::Aabb get_aabb() const;

   private:
    void on_transform_updated(entt::registry& reg, entt::entity e);
};

}  // namespace game
TMT_OBJECT(game::CollisionTrigger, (collision_shape));
