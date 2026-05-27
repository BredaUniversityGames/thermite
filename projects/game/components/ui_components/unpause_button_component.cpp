#include "unpause_button_component.hpp"
#include "projects/game/components/managers/menu_controller.hpp"

namespace game {
void UnpauseButtonComponent::start() {
    auto menu_controller_view = tmt::engine.ecs.view<MenuController>();
    if (menu_controller_view.empty) {
        tmt::Log::error("Menu controller not found. Please ensure a menu controller is present to use the unpause button component.");
        return;
    }
    menu_controller_entity = menu_controller_view.front().entity;
}

void UnpauseButtonComponent::update(const tmt::FrameData& time) {
    
}

void UnpauseButtonComponent::end() {
    
}

void UnpauseButtonComponent::unpause(tmt::Button::Context context) {
    
}
}