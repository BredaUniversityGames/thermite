#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "goap_actions/chase_player.hpp"
#include "goap_actions/kill_player.hpp"
#include "goap_actions/patrol_area.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {
        auto& action_reg = tmt::GoapActionRegistry::instance();
        auto& goal_reg = tmt::GoapGoalRegistry::instance();

        // --- Register Actions ---
        action_reg.register_action(std::make_unique<tmt::PatrolArea>());
        action_reg.register_action(std::make_unique<tmt::ChasePlayer>());
        action_reg.register_action(std::make_unique<tmt::KillPlayer>());

        // --- Register Goals ---
        {
            tmt::GoapGoal patrol;
            patrol.name = "PatrolArea";
            patrol.desired_state = {{tmt::FactId("area_secure"), tmt::FactValue(true)}};
            patrol.priority = 1;
            patrol.valid = true;

            goal_reg.register_goal("PatrolArea", patrol);
        }

        {
            tmt::GoapGoal kill;
            kill.name = "KillPlayer";
            kill.desired_state = {{tmt::FactId("player_alive"), tmt::FactValue(false)}};
            kill.priority = 10;
            kill.valid = true;

            goal_reg.register_goal("KillPlayer", kill);
        }

        // --- Register Agent Types ---
        auto& type_reg = tmt::GoapAgentTypeRegistry::instance();

        tmt::GoapAgentType enemy1;
        enemy1.id = "Enemy 1";

        enemy1.action_ids = {"PatrolArea", "ChasePlayer", "KillPlayer"};

        enemy1.goal_ids = {"PatrolArea", "KillPlayer"};

        enemy1.default_world_state = {
            {std::hash<std::string>()("player_visible"), true},
            {std::hash<std::string>()("player_in_range"), false},
            {std::hash<std::string>()("player_alive"), true},
            {std::hash<std::string>()("area_secure"), false}
        };

        type_reg.register_type(enemy1);

        tmt::GoapAgentType enemy2;
        enemy2.id = "Enemy 2";

        enemy2.action_ids = {"PatrolArea"};

        enemy2.goal_ids = {"PatrolArea"};

        enemy2.default_world_state = {{std::hash<std::string>()("area_secure"), false}};

        type_reg.register_type(enemy2);
    }

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class AIScene : public tmt::Scene<AIScene> {
   public:
    static constexpr std::string_view scene_name() { return "AIScene"; }

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

    tmt::engine.scenes.register_scene<AIScene>();

    return std::make_unique<Game>(specs);
}

void AIScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

    auto& ecs = tmt::engine.ecs;

    {
        // Spawn agents from type registry via factory
        tmt::Entity ai1 = tmt::GoapAgentFactory::spawn_agent_from_type("Enemy 1");
        tmt::Entity ai2 = tmt::GoapAgentFactory::spawn_agent_from_type("Enemy 1");

        // Set unique positions
        ecs.get_component<tmt::Transform>(ai1).set_world_position({0.f, 0.f, 0.f});
        ecs.get_component<tmt::Transform>(ai2).set_world_position({5.f, 0.f, 0.f});
    }
}

void AIScene::on_update(const tmt::FrameData& time) {}

void AIScene::on_end() {}
