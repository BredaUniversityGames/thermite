#pragma once
#include "engine/events/game.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {
namespace AnimConstraints {
class BaseAnimConstraint : OnGameStart, OnGameUpdate {
   public:
    // Inherited via OnGameStart
    void on_game_start() override;

    // Inherited via OnGameUpdate
    void on_game_update(const FrameData& time) override;
};

class DampedTransformConstraint : BaseAnimConstraint {
   public:
    void on_game_start() override;
    void on_game_update(const tmt::FrameData& time) override;

    tmt::Entity start_entity;
    float damp = 0.f;
   private:
    struct LocalRestPose {
        glm::vec3 local_pos;
        glm::quat local_rot;
        tmt::Entity bone_entity;

        glm::vec3 prev_world_pos;
        glm::quat prev_world_rot;

        std::vector<LocalRestPose> children_rest_poses;
    } local_rest_pose; // root

    void set_local_rest_pose(const auto& transform, LocalRestPose& rest_pose);
    void update_pose(LocalRestPose& rest_pose, const glm::vec3& parent_world_pos, const glm::quat& parent_world_rot);
};
}  // namespace AnimConstraints
}  // namespace tmt
