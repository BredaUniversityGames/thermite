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

#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal.hpp"

/* Components */
#include "components/player.hpp"
#include "components/asteroid_spawner.hpp"
#include "components/animation_player.hpp"

class Demo : public tmt::Application {
   public:
    Demo(const tmt::ApplicationSpecs& specs) : Application(specs) {}
};

class MainScene : public tmt::Scene<MainScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainScene"; }
};

class 

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Demo",
        .command_args = args,
        .log_file = "demo_logs.txt",
    };

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainScene>();

    /*Regsiter Components */
    tmt::engine.component_registry.register_component<Player>();
    tmt::engine.component_registry.register_component<AsteroidSpawner>();
    tmt::engine.component_registry.register_component<AnimationPlayer>();

    /* Register actions */
    auto& action_reg = tmt::GoapActionRegistry::instance();
    auto& goal_reg = tmt::GoapGoalRegistry::instance();




    return std::make_unique<Demo>(specs);
}