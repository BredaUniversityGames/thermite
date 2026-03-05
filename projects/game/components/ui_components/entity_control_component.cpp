#include "entity_control_component.hpp"

void game::EntityControlComponent::start() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding switch scene to on click.");
        button_component->on_click.add(this, &EntityControlComponent::enable);
        button_component->on_click.add(this, &EntityControlComponent::disable);
    } else {
        tmt::Log::warn("No button found for entity control component.");
    }
}

void game::EntityControlComponent::update(const tmt::FrameData& time) {
    
}

void game::EntityControlComponent::end() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

void game::EntityControlComponent::disable() {
    for (auto entity_to_disable : entities_to_disable) {
        if (tmt::engine.ecs.valid(entity_to_disable)) {
            tmt::engine.ecs.disable(entity_to_disable);
        }
    }
}

void game::EntityControlComponent::enable() {
    for (auto entity_to_enable : entities_to_enable) {
        if (tmt::engine.ecs.valid(entity_to_enable)) {
            tmt::engine.ecs.enable(entity_to_enable);
        }
    }
}