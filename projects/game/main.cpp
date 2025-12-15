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

#include <deque>

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    //{ /* Voxel entity */
    //    voxel = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
    //    renderer.size = glm::uvec3(20u, 10u, 10u);
    //    transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
    //    transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
    //    transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    //}
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
        //renderer.size = glm::uvec3(20u, 20u, 20u);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    { /* Voxel Physics Entity */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume;

        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume;

        vb.gravity = 0.5f;
        vb.type = tmt::VoxelBody::DYNAMIC;
        // renderer.size = glm::uvec3(20u, 20u, 20u);
        transform.set_world_position(glm::vec3(0.5f, 50.0f, 0.0f));
    }

    ///* Voxel Physics Entity */ //9998
    //for (int i = 0; i < 0; i++) { 
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume;
    //    //vb.gravity = 9.81f;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::DYNAMIC;
    //    //renderer.size = glm::uvec3(10u, 10u, 10u);
    //    glm::vec3 random_pos = glm::vec3(static_cast<float>(rand() % 10000) / 10000.0f - 0.5f, static_cast<float>(rand() % 10000) / 10000.0f, static_cast<float>(rand() % 10000) / 10000.0f - 0.5f);

    //    transform.set_world_position(random_pos * 100.0f);
    //}

    //{ /* Voxel Physics Entity */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    
    //    vb.gravity = 2.0f;
    //    vb.type = tmt::VoxelBody::DYNAMIC;
    //    vb.density = 100.0f;
    //    //renderer.size = glm::uvec3(10u, 10u, 10u);
    //    transform.set_world_position(glm::vec3(-0.4f, 90.25f, 10.0f));
    //}

    // constexpr float PRIM_RANGE = 128.0f;
    // for (int i = 0; i < 255; ++i) {
    //     auto entity = tmt::engine.ecs.create_entity();
    //     auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //     auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //     renderer.size = glm::uvec3(10u + (rand() % 90u), 10u + (rand() % 90u), 10u + (rand() % 90u));
    //     float rx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float ry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rz = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rrx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     float rry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     transform.set_world_position(glm::vec3(rx, ry, rz));
    //     transform.set_world_rotation(glm::vec3(glm::radians(rrx), glm::radians(rry), 0.0f));
    //     transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(sinf(time_passed), cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
