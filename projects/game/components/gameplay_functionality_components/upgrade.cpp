#include "upgrade.hpp"

#include "fuel.hpp"
#include "engine/core/components/button.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/mining_component.hpp"
#include "player.hpp"
#include "projects/game/components/development_tools/save_data.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"

namespace game {

void Upgrade::start() {
    auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    if (button_component) {
        tmt::Log::info("Found button component, adding apply function to button.");
        button_component->on_click.add(this, &Upgrade::button_apply);
    } else {
        tmt::Log::warn("No button found for upgrade!");
    }
}

void Upgrade::end() {
    auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    if (button_component) {
        button_component->on_click.clear();
    }
}

void Upgrade::button_apply() {
    if (apply_upgrade()) {
        tmt::engine.ecs.disable(entity);
        tmt::Log::info("Applied upgrade.");

    } else {
        tmt::Log::warn("Did not apply upgrade.");
    }
}

void Upgrade::modify_upgrade_entities() const {
    switch (type) {
        case UpgradeType::MAX_HEALTH:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_HEALTH_DATA).max_value = upgrade_to;
            break;
        case UpgradeType::MAX_BATTERY:
            tmt::engine.player_data.get<PlayerStat>(PLAYER_ENERGY_DATA).max_value = upgrade_to;
            break;
        case UpgradeType::MAX_SPEED:
            tmt::engine.player_data.get<PlayerMovement>(PLAYER_MOVEMENT_DATA).max_speed = upgrade_to;
            break;
        case UpgradeType::ACCELERATION:
            tmt::engine.player_data.get<PlayerMovement>(PLAYER_MOVEMENT_DATA).acceleration = upgrade_to;
            break;
        case UpgradeType::BARGE_MAX_FUEL:
            tmt::engine.player_data.get<FuelData>(FUEL_DATA).max_fuel = upgrade_to;
            break;
        case UpgradeType::MINING_SPEED:
            tmt::Log::warn("Not implemented yet {}", magic_enum::enum_name(type));
            break;
        case UpgradeType::GRAVITY_PULL:
            tmt::Log::warn("Not implemented yet {}", magic_enum::enum_name(type));
            break;
        case UpgradeType::EXPLOSION_PARAMETERS:
            tmt::Log::warn("Not implemented yet {}", magic_enum::enum_name(type));
            break;
        default:
            tmt::Log::warn("Not implemented yet {}", magic_enum::enum_name(type));
            break;
    }
}
bool Upgrade::apply_upgrade() {
    auto& currencies { tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) };

    for (auto& [resource, cost] : new_upgrade_costs) {
        if (currencies.resource_counts[resource] < cost) {
            tmt::Log::info("Unable to buy upgrade, insufficient {}.", magic_enum::enum_name(resource));
            return false;
        }
    }
    if (currencies.dollars <= dollar_cost) {
        tmt::Log::info("Unable to buy upgrade, insufficient {} dollars", currencies.dollars);
        return false;
    }

    for (auto& [resource, cost] : new_upgrade_costs) {
        currencies.resource_counts[resource] -= cost;
    }
    currencies.dollars -= dollar_cost;

    modify_upgrade_entities();

    return true;
}

}  // namespace game
