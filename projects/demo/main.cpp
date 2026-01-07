#include "engine/entry_point.hpp"
#include <glm/glm.hpp>
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/camera/camera_system.hpp"

/* Components */
#include "components/player.hpp"
#include "components/asteroid_spawner.hpp"

class Demo : public tmt::Application {
   public:
    Demo(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class MainScene : public tmt::Scene<MainScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Demo",
        .command_args = args,
        .log_file = "demo_logs.txt",
    };

    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainScene>();

    /*Regsiter Components */
    tmt::engine.component_registry.register_component<Player>();
    tmt::engine.component_registry.register_component<AsteroidSpawner>();

    return std::make_unique<Demo>(specs);
}

void generate_random_entities(const std::vector<tmt::ResourceRef<tmt::VoxelVolume>>& voxel_volumes);

void MainScene::on_start() {
    std::vector<tmt::ResourceRef<tmt::VoxelVolume>> voxel_volumes;

    auto scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_0.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_1.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_2.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_3.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_4.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_5.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_6.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));
    scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "assets/asteroids/test_asteroid_7.vengi"});
    voxel_volumes.push_back(tmt::engine.resources.copy_resource<tmt::VoxelVolume>(scene));

    generate_random_entities(voxel_volumes);
}

void MainScene::on_update(const tmt::FrameData& time) {}

void MainScene::on_end() {}

/* Helper function */
void generate_random_entities(const std::vector<tmt::ResourceRef<tmt::VoxelVolume>>& voxel_volumes) {
    srand(0);
    constexpr float PRIM_RANGE = 512.0f;

    for (int i = 0; i < 255; ++i) {
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);

        int index = rand() % voxel_volumes.size();
        renderer.resource = voxel_volumes[index];

        float s = (float)rand() / RAND_MAX * 3.0f;
        float rx = (float)rand() / RAND_MAX * PRIM_RANGE - PRIM_RANGE / 2.0f;
        float ry = (float)rand() / RAND_MAX * PRIM_RANGE - PRIM_RANGE / 2.0f;
        float rz = (float)rand() / RAND_MAX * PRIM_RANGE - PRIM_RANGE / 2.0f;

        transform.set_world_position({rx, ry, rz});
        transform.set_world_scale(1.0f + glm::vec3(s));
    }
}