#pragma once
#include "player.hpp"
#include "wallet.hpp"

enum class UpgradeType : uint8_t { MAX_HEALTH = 0u, MAX_BATTERY = 1u, MAX_SPEED = 2u, ACCELERATION = 3u };

enum class UpgradeResource : uint8_t { DOLLARS = 0u, GOLD = 1u, SILVER = 2u, COPPER = 3u, IRON = 4u, ENEMY_CORES = 5u };

class Upgrade : public tmt::GameComponent<Upgrade> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade"; }

    // added {} to prevent warnings about missing function bodies
    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    UpgradeType type = UpgradeType::MAX_HEALTH;
    std::vector<std::tuple<UpgradeResource, float>> upgrade_costs;
    float upgrade_value = 1.0f;

    bool apply_upgrade();

   private:
    tmt::Entity player_entity = entt::null;
};
TMT_OBJECT(Upgrade, (type, upgrade_costs, upgrade_value));
