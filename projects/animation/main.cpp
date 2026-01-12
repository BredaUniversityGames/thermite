#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/animation/animation_system.hpp"
#include "engine/systems/camera/camera_system.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/systems/animation/rig_renderer.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/tools/test_motion.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class AnimationScene : public tmt::Scene<AnimationScene> {
   public:
    static constexpr std::string_view scene_name() { return "AnimationScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity parent_entity;
    tmt::Entity snake_entity;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();
    tmt::engine.scenes.register_scene<AnimationScene>();

    return std::make_unique<Game>(specs);
}



void AnimationScene::on_start() {
    using namespace tmt;
    { /* Camera entity */
        Entity entity = engine.ecs.create_entity("Camera");
        auto& transform = engine.ecs.get_component<Transform>(entity);
        auto& camera = engine.ecs.add_component<Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }

    { /* Animation entity */
        auto rig_ent = engine.ecs.create_entity();
        auto& rig_ent_rig_model_comp = engine.ecs.add_component<RigModel>(rig_ent);

        IO::FileLocation location {IO::Location::PROJECT, "Victory_animation.fbx"};
        rig_ent_rig_model_comp.init(location, rig_ent);
        //rig_ent_rig_model_comp.data->animation_files.push_back(location);
      //  rig_ent_rig_model_comp.data->reload();

       // rig_ent_rig_model_comp.set_current_animation("Victory_animation");
        //rig_ent_rig_model_comp.state = RigModel::State::ANIMATE_LOOP;
       // rig_ent_rig_model_comp.time = 0.0f;
    }


    { /* Test rig */
        parent_entity = engine.ecs.create_entity();
        engine.ecs.add_component<BoneHierarchyRenderer>(parent_entity);
        auto& parent_transform = engine.ecs.get_component<Transform>(parent_entity);
        parent_transform.set_local_position(glm::vec3(5.f, 0.f, 0.f));

        auto child_0 = engine.ecs.create_entity();
        auto& child_0_transform = engine.ecs.get_component<Transform>(child_0);
        child_0_transform.set_local_position(glm::vec3(5.f, 2.f, -1.f));

        auto child_1 = engine.ecs.create_entity();
        auto& child_1_transform = engine.ecs.get_component<Transform>(child_1);
        child_1_transform.set_local_position(glm::vec3(5.f, 4.f, -4.f));

         auto child_2 = engine.ecs.create_entity();
        auto& child_2_transform = engine.ecs.get_component<Transform>(child_2);
        child_2_transform.set_local_position(glm::vec3(5.f, 5.f, -7.f));

         auto child_3 = engine.ecs.create_entity();
        auto& child_3_transform = engine.ecs.get_component<Transform>(child_3);
        child_3_transform.set_local_position(glm::vec3(5.f, 6.f, -10.0f));

         auto child_4 = engine.ecs.create_entity();
        auto& child_4_transform = engine.ecs.get_component<Transform>(child_4);
        child_4_transform.set_local_position(glm::vec3(5.f, 6.5f, -12.f));

        parent_transform.add_child(child_0);
        child_0_transform.add_child(child_1);
        child_1_transform.add_child(child_2);
        child_2_transform.add_child(child_3);
        child_3_transform.add_child(child_4);
    }
    { /* snake rig */
        snake_entity = engine.ecs.create_entity();
        engine.ecs.add_component<BoneHierarchyRenderer>(snake_entity);
        auto& parent_transform = engine.ecs.get_component<Transform>(snake_entity);
        parent_transform.set_local_position(glm::vec3(0.f, -3.f, -10.f));

        auto child_0 = engine.ecs.create_entity();
        auto& child_0_transform = engine.ecs.get_component<Transform>(child_0);
        child_0_transform.set_local_position(glm::vec3(-2.f, -3.f, -10.f));

        auto child_1 = engine.ecs.create_entity();
        auto& child_1_transform = engine.ecs.get_component<Transform>(child_1);
        child_1_transform.set_local_position(glm::vec3(-4.f, -3.f, -10.f));

        auto child_2 = engine.ecs.create_entity();
        auto& child_2_transform = engine.ecs.get_component<Transform>(child_2);
        child_2_transform.set_local_position(glm::vec3(-6.f, -3.f, -10.f));

        auto child_3 = engine.ecs.create_entity();
        auto& child_3_transform = engine.ecs.get_component<Transform>(child_3);
        child_3_transform.set_local_position(glm::vec3(-8.f, -3.f, -10.0f));

        auto child_4 = engine.ecs.create_entity();
        auto& child_4_transform = engine.ecs.get_component<Transform>(child_4);
        child_4_transform.set_local_position(glm::vec3(-10.f, -3.f, -10.f));

        parent_transform.add_child(child_0);
        child_0_transform.add_child(child_1);
        child_1_transform.add_child(child_2);
        child_2_transform.add_child(child_3);
        child_3_transform.add_child(child_4);
    }
}

void AnimationScene::on_update(const tmt::FrameData& time) 
{
    using namespace tmt;

    auto& snake_transform = engine.ecs.get_component<Transform>(snake_entity);

    snake_transform.set_local_position(TestMotion::sample_position(time.elapsed_time));
    snake_transform.set_local_rotation(TestMotion::sample_rotation(time.elapsed_time, time.delta_time));
}

void AnimationScene::on_end() {}
