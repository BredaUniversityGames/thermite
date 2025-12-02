#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"

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
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

    { /* Voxel entity */
        voxel = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        renderer.size = glm::uvec3(20u, 10u, 10u);
        transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
        transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    transform.set_world_rotation(glm::vec3(0.0f, cosf(time_passed), 0.0f));
    transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
