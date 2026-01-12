#include "animation_system.hpp"

#include "core/logger.hpp"

#include "engine.hpp"
#include "rig_model.hpp"
#include "rig_renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"

#define CONDITION(comparison) [](const Variant& parameter, const Variant& value) -> bool { return parameter comparison value; }

using namespace tmt;

std::string tmt::RigModelManager::get_name() { return "animation system"; }

void tmt::RigModelManager::on_start() {}

void RigModelManager::on_update(const FrameData& time) {
    inspect(time.delta_time);
    for (const auto& [entity, rig] : engine.ecs.get_registry().view<RigModel>().each()) {
        if (rig.state != RigModel::State::STATIONARY && !rig.is_transferring()) rig.time += rig.animation_speed * time.delta_time;

        if (rig.is_transferring()) rig.transfer_time += time.delta_time * rig.animation_speed;

        if (rig.transfer_time > rig.transfer_threshold) {
            switch (rig.state) {
                case RigModel::State::TRANSFERRING_TO_LOOP:
                    rig.state = RigModel::State::ANIMATE_LOOP;
                    break;
                case RigModel::State::TRANSFERRING_TO_ONCE:
                    rig.state = RigModel::State::ANIMATE_ONCE;
                    break;
                case RigModel::State::TRANSFERRING_TO_STOP:
                    rig.state = RigModel::State::STATIONARY;
                    break;

                case RigModel::State::ANIMATE_LOOP:
                case RigModel::State::ANIMATE_ONCE:
                case RigModel::State::STATIONARY:
                    break;
            }

            rig.time = 0.0f;
            rig.set_current_animation(rig.get_next_animation());
            rig.transfer_time = 0.0f;
        }

        const Bone& last_bone = rig.data->bones.back();
        const std::string& current_animation = rig.get_current_animation();
        if (last_bone.animations.find(current_animation) == last_bone.animations.end()) {
            //Log::warn(R"(Animation "{}" isn't a valid animation!)", current_animation);
            continue;
        }

        const float animation_time = last_bone.animations.at(rig.get_current_animation()).keyframes_pos.back().time;

        if (rig.time >= animation_time) {
            if (rig.state == RigModel::State::ANIMATE_ONCE) {
                rig.state = RigModel::State::STATIONARY;
            } else if (rig.state == RigModel::State::ANIMATE_LOOP) {
                rig.time = rig.time - animation_time;
            }
        }

        for (const Entity bone_entity : rig.bone_entities) {
            auto& transform = engine.ecs.get_component<Transform>(bone_entity);

            const BoneComp& bone_comp_id = engine.ecs.get_component<BoneComp>(bone_entity);
            Bone& bone = rig.data->bones[bone_comp_id.id];

            rig.animate_translation(transform, bone);
            rig.animate_rotation(transform, bone);
            rig.animate_scale(transform, bone);
        }
    }
}

void tmt::RigModelManager::on_end() {}

void RigModelManager::inspect(float) {
    int lines_drawn = 0;
    engine.polyline.use_color(1.0f, 1.0f, 1.0f);
    engine.polyline.use_line_width(0.25f);
    for (const auto& [entity, transform, rig] : engine.ecs.get_registry().view<Transform, RigModel>().each()) {
        //for (const Entity bone_entity : rig.bone_entities) {
        //    auto& bone_transform = engine.ecs.get_component<Transform>(bone_entity);
        //    // auto& bone_name = engine.ecs.get_component<Name>(bone_entity);
        //    const glm::vec3 joint = bone_transform.get_world_position();

        //    if (bone_transform.has_parent()) {
        //        auto& parent_transform = engine.ecs.get_component<Transform>(bone_transform.get_parent());
        //        const glm::vec3 parent_joint = parent_transform.get_world_position();
        //        engine.polyline.draw_line(parent_joint, joint);
        //        lines_drawn++;
        //    }
        //}

        
    }

    for (const auto& [entity, transform, bone_renderer] : engine.ecs.get_registry().view<Transform, BoneHierarchyRenderer>().each()) 
    {
        tmt::engine.polyline.use_color(bone_renderer.color);
        tmt::engine.polyline.use_line_width(bone_renderer.line_width);
        tmt::RenderBoneHierarchy(transform);
    }
}
void tmt::ConstraintSystem::on_start() 
{
    for (const auto& [entity, transform, damped_constraint] : engine.ecs.get_registry().view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        damped_constraint.local_rest_pose.bone_entity = entity;
        auto& start_transform = tmt::engine.ecs.get_component<Transform>(entity);

        set_local_rest_pose(start_transform, damped_constraint.local_rest_pose);
    }
}
void tmt::ConstraintSystem::on_update(const tmt::FrameData& time) 
{
    for (const auto& [entity, transform, damped_constraint] : engine.ecs.get_registry().view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        auto& start_transform = tmt::engine.ecs.get_component<Transform>(entity);

        damped_constraint.local_rest_pose.local_pos = start_transform.get_local_position();
        damped_constraint.local_rest_pose.local_rot = start_transform.get_local_rotation();

        update_damped_pose(damped_constraint.local_rest_pose, damped_constraint.local_rest_pose.local_pos, damped_constraint.local_rest_pose.local_rot, damped_constraint.damp);
    }
    
}
void tmt::ConstraintSystem::set_local_rest_pose(const tmt::Transform& transform, AnimConstraints::LocalRestPose& rest_pose) {
    rest_pose.local_rot = transform.get_local_rotation();
    rest_pose.local_pos = transform.get_local_position();
    rest_pose.prev_world_rot = transform.get_world_rotation();
    rest_pose.prev_world_pos = transform.get_world_position();

    const auto& children = transform.get_children();

    for (auto child : children) {
        const auto& child_transform = engine.ecs.get_component<Transform>(child);

        AnimConstraints::LocalRestPose child_rest_pose;
        child_rest_pose.bone_entity = child;
        rest_pose.children_rest_poses.push_back(child_rest_pose);

        set_local_rest_pose(child_transform, rest_pose.children_rest_poses.back());
    }
}

void tmt::ConstraintSystem::update_damped_pose(AnimConstraints::LocalRestPose& local_rest_pose, const glm::vec3& parent_world_pos, const glm::quat& parent_world_rot, float damp) {
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

        update_damped_pose(child_pose, new_world_pos, new_world_rot, damp);
    }
}


void tmt::ConstraintSystem::on_end() {}

std::string tmt::ConstraintSystem::get_name() { return std::string(); }
