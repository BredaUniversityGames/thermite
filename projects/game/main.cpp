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
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/systems/gameplay/game_component.hpp"

// For custom ImGui logic
#include "editor/all.hpp"

// Game Components
#include "components/player.hpp"
#include "components/wallet.hpp"
#include "components/asteroid_field_component.hpp"
#include "components/mining_component.hpp"
#include "components/animation_player.hpp"
#include "components/attach_component.hpp"
#include "components/gravity_manipulation_component.hpp"
#include "components/collision_trigger.hpp"
#include "components/menu_controller.hpp"
#include "components/mover_component.hpp"
#include "components/ore_collector.hpp"
#include "components/projectile_spawner.hpp"
#include "components/rifle_projectile.hpp"
#include "components/spawner.hpp"
#include "components/text_component.hpp"
#include "components/weapon.hpp"
#include "components/fuel.hpp"
#include "components/ore_properties.hpp"
#include "components/upgrade.hpp"

// Goap Components
#include "engine/systems/ai/goap/goap_system.hpp"
#include "components/goap_actions/steer_to_player.hpp"
#include "components/goap_actions/wander_steering.hpp"
#include "components/goap_actions/explode.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class MainMenuScene : public tmt::Scene<MainMenuScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainMenuScene"; }
};

class MainGameScene : public tmt::Scene<MainGameScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainGameScene"; }
};
class Zoo : public tmt::Scene<Zoo> {
   public:
    static constexpr std::string_view scene_name() { return "Zoo"; }
};
class Gym : public tmt::Scene<Gym> {
   public:
    static constexpr std::string_view scene_name() { return "Gym"; }
};

class DanielTestScene : public tmt::Scene<DanielTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "DanielTestScene"; }
};

class MikaTestScene : public tmt::Scene<MikaTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "MikaTestScene"; }
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Mining Game",
        .command_args = args,
        .log_file = "mining_game_logs.txt"
    };
    // clang-format on

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainGameScene>();
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MikaTestScene>();
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MainMenuScene>();
    tmt::engine.scenes.register_scene<Gym>();
    tmt::engine.scenes.register_scene<Zoo>();

    /* Register Game Components */
    tmt::engine.component_registry.register_component<game::Player>();
    tmt::engine.component_registry.register_component<game::Weapon>();
    tmt::engine.component_registry.register_component<game::Spawner>();
    tmt::engine.component_registry.register_component<game::RifleProjectile>();
    tmt::engine.component_registry.register_component<game::ProjectileSpawner>();
    tmt::engine.component_registry.register_component<game::AsteroidFieldComponent>();
    tmt::engine.component_registry.register_component<game::Wallet>();
    tmt::engine.component_registry.register_component<game::MiningComponent>();
    tmt::engine.component_registry.register_component<game::AnimationPlayer>();
    tmt::engine.component_registry.register_component<game::GravityManipulationComponent>();
    tmt::engine.component_registry.register_component<game::OreCollector>();
    tmt::engine.component_registry.register_component<game::CollisionTrigger>();
    tmt::engine.component_registry.register_component<game::WeaponManager>();
    tmt::engine.component_registry.register_component<game::TextComponent>();
    tmt::engine.component_registry.register_component<game::MoverComponent>();
    tmt::engine.component_registry.register_component<game::AttachComponent>();
    tmt::engine.component_registry.register_component<game::Upgrade>();
    tmt::engine.component_registry.register_component<game::MenuController>();
    tmt::engine.component_registry.register_component<game::FuelComponent>();
    tmt::engine.component_registry.register_component<game::OreProperties>();

    /* Register Goap Components */
    {
        if (!tmt::engine.ecs.systems.try_get<tmt::Goap>()) {
            tmt::engine.ecs.systems.add<tmt::Goap>();
        }

        auto& ecs = tmt::engine.ecs;
        auto& goap = ecs.systems.get<tmt::Goap>();
        
        auto& action_reg = goap.actions();
        auto& goal_reg = goap.goals();
        auto& type_reg = goap.agent_types();

        /* Register Actions */
        action_reg.register_action(std::make_unique<game::SteerToPlayer>());
        action_reg.register_action(std::make_unique<game::WanderSteering>());
        action_reg.register_action(std::make_unique<game::Explode>());

        /* Register Goals */

        // Goal: Wander when player not in range
        {
            tmt::GoapGoal wander_steering;
            wander_steering.name = "g_WanderSteering";
            wander_steering.desired_state = { { tmt::FactId("wandering"), true } };
            wander_steering.priority = 1;
            wander_steering.valid = true;
            goal_reg.register_goal("g_WanderSteering", wander_steering);
        }
        // Goal: Steer to player, get within explosion range
        {
            tmt::GoapGoal steer_to_player;
            steer_to_player.name = "g_SteerToPlayer";
            steer_to_player.desired_state = { { tmt::FactId("at_target"), true } };
            steer_to_player.priority = 5;
            steer_to_player.valid = true;
            goal_reg.register_goal("g_ReachTarget", steer_to_player);
        }
        // Goal: Explode when in explosion range
        {
            tmt::GoapGoal explode;
            explode.name = "g_Explode";
            explode.desired_state = { { tmt::FactId("exploded"), true } };
            explode.priority = 10;
            explode.valid = true;
            goal_reg.register_goal("g_Explode", explode);
        }
    }

    return std::make_unique<Game>(specs);
}
