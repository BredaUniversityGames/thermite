#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/camera/camera_system.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override;
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class DragonScene : public tmt::Scene {
   public:
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    std::string get_name() const override { return "Dragon Scene"; }
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.add<DragonScene>();

    return std::make_unique<Game>(specs);
}

void Game::on_start() {}

void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "dragon128.vengi"});
    auto voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);

    { /* Voxel Physics Entity */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume;

        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        // renderer.size = glm::uvec3(20u, 20u, 20u);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, 0.0f));
        transform.set_world_rotation(glm::vec3(3.1415f, 0.0f, 0.0f));
    }

    { /* Voxel Physics Entity */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume;

        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume;

        vb.gravity = 2.0f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        // renderer.size = glm::uvec3(20u, 20u, 20u);
        transform.set_world_position(glm::vec3(0.5f, 30.0f, 0.0f));
    }
}

void DragonScene::on_update(const tmt::FrameData& time) {}

void DragonScene::on_end() {}
