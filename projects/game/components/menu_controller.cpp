#include "menu_controller.hpp"
#include "engine/core/input/input.hpp"
#include "game_input.hpp"

namespace game {

void MenuController::end() {}

void MenuController::update(const tmt::FrameData& time) {
    auto& input = tmt::engine.input;

    if (input.is_action_just_pressed(action::OPEN_INVENTORY)) {
        if (tmt::engine.ecs.is_disabled(inventory_menu_entity)) {
            enable_inventory_menu();
        } else {
            disable_inventory_menu();
        }
    }

    if (input.is_action_just_pressed(action::OPEN_PAUSE_MENU)) {
        if (tmt::engine.ecs.is_disabled(pause_menu_entity)) {
            enable_pause_menu();
        } else {
            disable_pause_menu();
        }
    }

    if (input.is_action_just_pressed(action::OPEN_UPGRADE_MENU)) {
        tmt::Log::info("Clicked upgrade menu button");

        if (tmt::engine.ecs.is_disabled(upgrade_menu_entity)){
            tmt::Log::info("Debug En");
            enable_upgrade_menu();
        } else {
            tmt::Log::info("Debug Dis");
            disable_upgrade_menu();
        }
    }
}

void MenuController::start() {}

void MenuController::enable_pause_menu() const {
    tmt::engine.ecs.enable(pause_menu_entity);
}

void MenuController::disable_pause_menu() const {
    tmt::engine.ecs.disable(pause_menu_entity);
}

void MenuController::enable_inventory_menu() const {
    tmt::engine.ecs.enable(inventory_menu_entity);
}

void MenuController::disable_inventory_menu() const {
    tmt::engine.ecs.disable(inventory_menu_entity);
}

void MenuController::enable_upgrade_menu() const {
    tmt::engine.ecs.enable(upgrade_menu_entity);
}

void MenuController::disable_upgrade_menu() const {
    tmt::engine.ecs.disable(upgrade_menu_entity);
}

}  // namespace game