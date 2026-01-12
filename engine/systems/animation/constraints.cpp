#include "constraints.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"

void tmt::AnimConstraints::BaseAnimConstraint::on_game_start() {}

void tmt::AnimConstraints::BaseAnimConstraint::on_game_update(const FrameData& time) {}

void tmt::AnimConstraints::DampedTransformConstraint::on_game_start() 
{
    local_rest_pose.bone_entity = start_entity;
	auto& start_transform = tmt::engine.ecs.get_component<Transform>(start_entity);
	
    set_local_rest_pose(start_transform, local_rest_pose);
}

void tmt::AnimConstraints::DampedTransformConstraint::on_game_update(const tmt::FrameData& time) 
{
    auto& start_transform = tmt::engine.ecs.get_component<Transform>(start_entity);

    local_rest_pose.local_pos = start_transform.get_local_position();
    local_rest_pose.local_rot = start_transform.get_local_rotation();

    update_pose(local_rest_pose, local_rest_pose.local_pos, local_rest_pose.local_rot);
}

void tmt::AnimConstraints::DampedTransformConstraint::set_local_rest_pose(const auto& transform, LocalRestPose& rest_pose) 
{
    rest_pose.local_rot = transform.get_local_rotation();
    rest_pose.local_pos = transform.get_local_position();
    rest_pose.prev_world_rot = transform.get_world_rotation();
    rest_pose.prev_world_pos = transform.get_world_position();

    const auto& children = transform.get_children();

    for (auto child : children) {
        const auto& child_transform = engine.ecs.get_component<Transform>(child);

        LocalRestPose child_rest_pose;
        child_rest_pose.bone_entity = child;
        rest_pose.children_rest_poses.push_back(child_rest_pose);

        set_local_rest_pose(child_transform, rest_pose.children_rest_poses.back());
    }
}

void tmt::AnimConstraints::DampedTransformConstraint::update_pose(LocalRestPose& local_rest_pose, const glm::vec3& parent_world_pos, const glm::quat& parent_world_rot) 
{
    for (auto& child_pose : local_rest_pose.children_rest_poses) {
        glm::quat ws_target_rot = parent_world_rot * child_pose.local_rot;
        glm::vec3 ws_target_pos = parent_world_pos + (ws_target_rot * child_pose.local_pos);

        glm::vec3 new_world_pos = glm::mix(ws_target_pos, child_pose.prev_world_pos, damp);
        glm::quat new_world_rot = glm::slerp(ws_target_rot, child_pose.prev_world_rot, damp);

        glm::quat new_local_rot = glm::inverse(parent_world_rot) * new_world_rot;
        glm::vec3 new_local_pos = glm::inverse(parent_world_rot) * (new_world_pos - parent_world_pos);

        auto& child_transform = tmt::engine.ecs.get_component<Transform>(child_pose.bone_entity);

        child_transform.set_local_rotation(new_local_rot);
        child_transform.set_local_position(new_local_pos);

        child_pose.prev_world_pos = new_world_pos;
        child_pose.prev_world_rot = new_world_rot;

        update_pose(child_pose, new_world_pos, new_world_rot);
    }
}

