#pragma once
#include "engine/core/entity.hpp"
#include "glm/gtc/quaternion.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {
namespace AnimConstraints {

struct LocalRestPose {
    glm::vec3 local_pos;
    glm::quat local_rot;
    tmt::Entity bone_entity;

    glm::vec3 prev_world_pos;
    glm::quat prev_world_rot;

    std::vector<LocalRestPose> children_rest_poses;
};

struct DampedTransformConstraint{
    float damp = 0.f;
    LocalRestPose local_rest_pose; // root
};
struct TwoBoneIKConstraint
{
    tmt::Entity parent = entt::null;
    tmt::Entity root;
    tmt::Entity mid;
    tmt::Entity tip;

    glm::vec3 target_position;
    glm::vec3 bend_position;
};

}  // namespace AnimConstraints
}  // namespace tmt
TMT_COMPONENT(tmt::AnimConstraints::DampedTransformConstraint, "DampedTransformConstraint", (damp));