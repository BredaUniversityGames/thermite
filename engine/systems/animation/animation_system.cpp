#include "animation_system.hpp"

#include "core/logger.hpp"

#include "engine.hpp"
#include "rig_model.hpp"
#include "rig_renderer.hpp"
#include "animation_data.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/systems/animation/constraints/damped_transform.hpp"
#include "engine/systems/animation/constraints/two_bone_ik.hpp"

#define CONDITION(comparison) [](const Variant& parameter, const Variant& value) -> bool { return parameter comparison value; }

namespace tmt {

std::string tmt::RigModelManager::get_name() {
    return "animation system";
}

void tmt::RigModelManager::on_game_start() {
    for (const auto& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        auto& rigmodel = tmt::engine.ecs.get_component<tmt::RigModel>(entity);
        rigmodel.init(rigmodel.data.file_location, entity);
    }
}

void tmt::RigModelManager::on_start() {}

void RigModelManager::on_update(const FrameData& time) {
    inspect(time.delta_time);
    for (const auto& [entity, rig] : engine.ecs.view<RigModel>().each()) {
        auto path = rig.vox_path.relative_path;
        if (!rig.vox_is_loaded && !path.empty()) {
            rig.attach_voxel_objects();
        }

        if (rig.data == nullptr) continue;

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

        AnimConstraints::DampedTransform::set_local_rest_pose(start_transform, damped_constraint.local_rest_pose);
    }
}
void tmt::ConstraintSystem::on_update(const tmt::FrameData& time) 
{
    for (const auto& [entity, transform, damped_constraint] : engine.ecs.get_registry().view<Transform, AnimConstraints::DampedTransformConstraint>().each()) {
        auto& start_transform = tmt::engine.ecs.get_component<Transform>(entity);

        damped_constraint.local_rest_pose.local_pos = start_transform.get_local_position();
        damped_constraint.local_rest_pose.local_rot = start_transform.get_local_rotation();

        AnimConstraints::DampedTransform::update_damped_pose(damped_constraint.local_rest_pose, damped_constraint.local_rest_pose.local_pos, damped_constraint.local_rest_pose.local_rot, damped_constraint.damp);
    }
    for (const auto& [entity, transform, two_bone_constraint] : engine.ecs.get_registry().view<Transform, AnimConstraints::TwoBoneIKConstraint>().each()) {
        auto& start_transform = tmt::engine.ecs.get_component<Transform>(entity);

        AnimConstraints::TwoBoneIK::TwoBoneInputData input_data;
        if(engine.ecs.valid(two_bone_constraint.parent))
        {
            input_data.parent = &engine.ecs.get_component<Transform>(two_bone_constraint.parent);
        }

        auto& root_transform = engine.ecs.get_component<Transform>(two_bone_constraint.root);
        auto& mid_transform = engine.ecs.get_component<Transform>(two_bone_constraint.mid);

        input_data.root = &root_transform;
        input_data.mid = &mid_transform;
        input_data.end = &engine.ecs.get_component<Transform>(two_bone_constraint.tip);

        input_data.effector = two_bone_constraint.target_position;
        input_data.bend_pos = two_bone_constraint.bend_position;

        auto output = AnimConstraints::TwoBoneIK::solve_two_bone_ik(input_data);
        
        root_transform.set_local_rotation(output.root_rot);
        mid_transform.set_local_rotation(output.mid_rot);
    }
}



void tmt::ConstraintSystem::on_end() {}

std::string tmt::ConstraintSystem::get_name() { return std::string(); }

}