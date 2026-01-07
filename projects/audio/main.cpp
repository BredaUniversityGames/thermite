#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/resources/audio_bank.hpp"
#include "engine/core/components/audio_listener.hpp"

namespace {

std::vector<tmt::ResourceRef<tmt::AudioBank>> audio_banks;
tmt::Entity entity;
tmt::Entity entity2;
tmt::AudioInstance song_instance {nullptr};

}  // namespace

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}
};

class TestScene : public tmt::Scene<TestScene> {
   public:
    static std::string_view scene_name() { return "TestScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Audio Module Test",
        .command_args = args,
        .log_file = "audio.txt",
    };

    tmt::engine.scenes.register_scene<TestScene>();

    return std::make_unique<Game>(specs);
}

void TestScene::on_start() {
    entity = tmt::engine.ecs.create_entity();
    tmt::engine.ecs.get_component<tmt::Transform>(entity).set_world_position({0.0f, 0.0f, 0.0f});
    tmt::engine.ecs.add_component<tmt::AudioListener>(entity);

    entity2 = tmt::engine.ecs.create_entity();
    tmt::engine.ecs.get_component<tmt::Transform>(entity2).set_world_scale(glm::vec3 {0.1f});
    tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity2);
    const tmt::ResourceRef<tmt::VoxelScene> voxel_scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "dragon128.vengi"});
    const tmt::ResourceRef<tmt::VoxelVolume> voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_scene);
    renderer.resource = voxel_volume;

    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Master.bank"}, true));
    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Music.bank"}));
    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "SFX.bank"}));
    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Vehicles.bank"}));
    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "VO.bank"}));
    audio_banks.push_back(tmt::engine.resources.load_resource<tmt::AudioBank>({tmt::IO::Location::PROJECT, "Test.bank"}));

    song_instance = audio_banks.back()->get_audio_events().front().play();
}
void TestScene::on_update(const tmt::FrameData& time) {
    const float n = time.elapsed_time;
    const glm::vec3 position = glm::vec3 {glm::cos(n), 0.0f, glm::sin(n)} * 3.0f;

    tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity2);
    transform.set_world_position(position);
    song_instance.auto_set_3d_attributes(transform);
}
void TestScene::on_end() {}
