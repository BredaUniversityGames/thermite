#pragma once
#include "player.hpp"
#include "projects/game/data_headers/wallet.hpp"
namespace game {

enum class UpgradeType : uint8_t { MAX_HEALTH, MAX_BATTERY, MAX_SPEED, ACCELERATION, MINING_SPEED, GRAVITY_PULL, EXPLOSION_PARAMETERS, BARGE_MAX_FUEL };

class Upgrade : public tmt::GameComponent<Upgrade> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Upgrade"; }

    // added {} to prevent warnings about missing function bodies
    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override;

    UpgradeType type = UpgradeType::MAX_HEALTH;
    float upgrade_to = 1.0f;
    uint64_t dollar_cost = 1.0f;

    std::vector<std::tuple<OreProperties::OreResources, uint64_t>> new_upgrade_costs = { { OreProperties::OreResources::NONE, 1.0f } };  // An initial cost
    bool apply_upgrade();
    void button_apply();
    void modify_upgrade_entities() const;

   private:
};

}  // namespace game
TMT_OBJECT(game::Upgrade, (type, upgrade_to, dollar_cost, new_upgrade_costs));
