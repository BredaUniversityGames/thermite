#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "unordered_map"

namespace game {

struct RandomPrefabSpawner : public tmt::GameComponent<RandomPrefabSpawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Random Prefab Spawner"; }

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    std::vector<tmt::ResourceRef<tmt::Json>> prefabs;

    int spawn_limit = 1;
    float respawn_time = 20.0f;
    bool re_randomize = true;

   private:
    void spawn_prefab(const tmt::ResourceRef<tmt::Json>& prefab);
    std::unordered_map<tmt::Entity, tmt::ResourceRef<tmt::Json>> spawned_entities = {};
};

}  // namespace game
TMT_OBJECT(game::RandomPrefabSpawner, (prefabs, spawn_limit, respawn_time, re_randomize));
