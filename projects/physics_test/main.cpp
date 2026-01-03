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
#include "engine/systems/physics/physics_system.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    std::vector<tmt::Entity> voxels = {};
    int num = 0;
    float time_passed = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Physics Test",
        .command_args = args,
        .log_file = "physics_test_logs.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    auto voxel_file_piece = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "piece_1.vengi"});
    auto voxel_volume_piece = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_piece);

    auto voxel_file_ass3 = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "test_asteroid_3.vengi"});
    auto voxel_volume_ass3 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass3);

    auto voxel_file_ass7 = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "test_asteroid_7.vengi"});
    auto voxel_volume_ass7 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass7);

    for (size_t z = 0; z < 5; z++) {
        for (size_t i = 0; i < 300; i++) { /* Voxel Physics Entity */
            auto entity = tmt::engine.ecs.create_entity();
            auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
            auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
            renderer.resource = voxel_volume_piece;

            auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
            vb.resource = voxel_volume_piece;

            vb.gravity = 0.0f;
            vb.type = tmt::VoxelBody::DYNAMIC;
            float random_x = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 150.0f;
            float random_y = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 150.0f;
            transform.set_world_position(glm::vec3(random_x, random_y, (5 - z) * 5.0f));

            float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
            float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
            float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
            transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
            voxels.push_back(entity);
        }
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(0.0f, 50.0f, 110.0f));

        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(40.0f, 20.0f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-24.0f, -20.0f, 90.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(50.0f, 35.0f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(20.0f, -35.0f, 120.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-20.0f, 55.0f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-14.0f, 17.0f, 70.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(19.0f, 0.0f, 90.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(54.0f, -48.0f, 75.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;

    if (time_passed > 0.005f) {
        if (num >= voxels.size()) return;
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(voxels[num++]);
        float random_x = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 50.0f;
        float random_y = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 50.0f;
        tmt::Physics::add_force(vb, glm::vec3(random_x, random_y, 35.0f));
        time_passed = 0.0f;
    }

    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(sinf(time_passed), cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
