#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Spawner : public tmt::GameComponent<Spawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name();

    void start() override;

    void spawn() const;
    void spawn(const tmt::IO::FileLocation& location, const tmt::Entity& parent = entt::null);
    // core state
    tmt::Entity spawn_parent { entt::null };
    tmt::IO::FileLocation prefab_location;
    // configurable
    bool use_attached_entity_as_spawn_parent = true;

   private:
};

}  // namespace game
