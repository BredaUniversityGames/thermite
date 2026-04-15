#pragma once
#include "player.hpp"
#include "projects/game/data_headers/wallet.hpp"
namespace game {

enum class UpgradeType : uint8_t { MAX_HEALTH = 0u, MAX_BATTERY = 1u, MAX_SPEED = 2u, ACCELERATION = 3u, PRIMARY_ATK_SPEED = 4u, SECONDARY_ATK_SPEED = 5u, GUN_DMG = 6u, BARGE_MAX_FUEL = 7u };
struct PersistentStats {
    float max_health = 0;
    float max_battery = 0;
    float max_speed = 0;
    float acceleration = 0;
    float primary_atk_speed = 0;
    float secondary_atk_speed = 0;
    float gun_dmg = 0;
    float barge_max_fuel = 0;
};
class Upgrade : public tmt::GameComponent<Upgrade> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade"; }

    // added {} to prevent warnings about missing function bodies
    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override;

    tmt::Entity upgrade_target = entt::null;
    UpgradeType type = UpgradeType::MAX_HEALTH;
    float upgrade_to = 1.0f;
    tmt::Entity player_entity = entt::null;
    uint64_t dollar_cost = 1.0f;
    std::vector<std::tuple<OreProperties::OreResources, uint64_t>> new_upgrade_costs = { { OreProperties::OreResources::NONE, 1.0f } };  // An initial cost
    bool apply_upgrade();
    void button_apply();
    bool modify_upgrade_entities() const;

   private:
};

}  // namespace game
TMT_OBJECT(game::Upgrade, (upgrade_target, type, upgrade_to, player_entity, dollar_cost, new_upgrade_costs));
TMT_OBJECT(game::PersistentStats, (max_health, max_battery, max_speed, acceleration, primary_atk_speed, secondary_atk_speed, gun_dmg, barge_max_fuel));
