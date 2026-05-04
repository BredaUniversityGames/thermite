#include "resource_bar_current.hpp"

#include "engine/core/components/text_renderer.hpp"

namespace game {
void ResourceBarCurrentComponent::start() {
    if (player_entity == entt::null) {
        player_entity = Player::get().entity;
    }

    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }

    if (resource == DisplayTypeResourceBar::HEALTH) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerHealthChanged>().connect<&ResourceBarCurrentComponent::health_changed>(this);
        resource_bar_update(static_cast<int>(player_component->health.value));
    } else if (resource == DisplayTypeResourceBar::ENERGY) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerEnergyChanged>().connect<&ResourceBarCurrentComponent::energy_changed>(this);
        resource_bar_update(static_cast<int>(player_component->energy.value));
    }
}

void ResourceBarCurrentComponent::update(const tmt::FrameData& time) {
}

void ResourceBarCurrentComponent::end() {
    if (resource == DisplayTypeResourceBar::HEALTH) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerHealthChanged>().disconnect<&ResourceBarCurrentComponent::health_changed>(this);
    } else if (resource == DisplayTypeResourceBar::ENERGY) {
        tmt::engine.ecs.get_dispatcher().sink<PlayerEnergyChanged>().disconnect<&ResourceBarCurrentComponent::energy_changed>(this);
    }
}

void ResourceBarCurrentComponent::health_changed(PlayerHealthChanged player_health_changed) {
    int new_val = static_cast<int>(player_health_changed.new_value);
    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }
    resource_bar_update(new_val);
}

void ResourceBarCurrentComponent::energy_changed(PlayerEnergyChanged player_energy_changed) {
    int new_val = static_cast<int>(player_energy_changed.new_value);
    auto* player_component = tmt::engine.ecs.try_get_component<Player>(player_entity);
    if (!player_component) {
        tmt::Log::error("Player component cannot be found for resource bar, please check entity: {} (resource bar) and {} (player)", entity, player_entity);
        return;
    }

    resource_bar_update(new_val);
}

void ResourceBarCurrentComponent::resource_bar_update(int new_val) {
    auto* text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);

    if (!text_renderer_component) {
        tmt::Log::error("Text renderer not found for resource bar, please check entity: {}", entity);
        return;
    }
    if (resource_per_segment == 0) {
        tmt::Log::error("Resource per segment cannot be 0, please check entity: {}", entity);
        return;
    }

    int displayed_bars = new_val / resource_per_segment;

    if (new_val > 0 && displayed_bars <= 0) {
        displayed_bars = 1;
    }

    std::string render_string;

    for (int i = 0; i < displayed_bars; i++) {
        render_string.append(rendered_string);
    }

    text_renderer_component->text = render_string;
}
} // namespace game