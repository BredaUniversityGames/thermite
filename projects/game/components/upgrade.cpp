#include "upgrade.hpp"
#include "player.hpp"

void Upgrade::start() {
    player_entity = Player::get_player_entity();  // Assuming there's only one player entity in the game
    if (player_entity == entt::null) {
        // something went wrong, no player found
    }
}

bool Upgrade::apply_upgrade() {
    if (player_entity == entt::null) {
        return false;  // no player entity in scene, return false
    }

    Player& player_component = tmt::engine.ecs.get_component<Player>(player_entity);
    Wallet& wallet_component = tmt::engine.ecs.get_component<Wallet>(player_entity);

    for (std::tuple<UpgradeResource, float> cost : upgrade_costs) {
        auto resource = std::get<0>(cost);
        auto upgrade_cost = std::get<1>(cost);
        switch (resource) {
            case UpgradeResource::DOLLARS:
                if (wallet_component.dollars < upgrade_cost) return false;
                wallet_component.dollars -= upgrade_cost;
                break;
            case UpgradeResource::GOLD:
                if (wallet_component.gold < upgrade_cost) return false;
                wallet_component.gold -= upgrade_cost;
                break;
            case UpgradeResource::SILVER:
                if (wallet_component.silver < upgrade_cost) return false;
                wallet_component.silver -= upgrade_cost;
                break;
            case UpgradeResource::COPPER:
                if (wallet_component.copper < upgrade_cost) return false;
                wallet_component.copper -= upgrade_cost;
                break;
            case UpgradeResource::IRON:
                if (wallet_component.iron < upgrade_cost) return false;
                wallet_component.iron -= upgrade_cost;
                break;
            case UpgradeResource::ENEMY_CORES:
                if (wallet_component.enemy_cores < upgrade_cost) return false;
                wallet_component.enemy_cores -= upgrade_cost;
                break;
            default:
                return false;  // Invalid resource type, cannot apply upgrade
        }
    }

    switch (type) {
        case UpgradeType::MAX_HEALTH:
            player_component.max_health += upgrade_value;
            break;
        case UpgradeType::MAX_BATTERY:
            player_component.max_battery += upgrade_value;
            break;
        case UpgradeType::MAX_SPEED:
            player_component.max_speed += upgrade_value;
            break;
        case UpgradeType::ACCELERATION:
            player_component.acceleration += upgrade_value;
            break;
        default:
            return false;  // Invalid upgrade type, cannot apply upgrade
    }

    return true;           // Upgrade applied successfully
}