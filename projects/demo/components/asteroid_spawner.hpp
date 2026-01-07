#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources/voxel_volume.hpp"

class AsteroidSpawner : public tmt::GameComponent<AsteroidSpawner> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AsteroidSpawner"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void on_inspect() override;

    void spawn_asteroid();

    uint32_t asteroid_count = 100;
    float spawn_radius = 500.0f;

    uint64_t seed = 42;

    std::vector<tmt::ResourceRef<tmt::VoxelVolume>> asteroid_bodies;
};
TMT_OBJECT(AsteroidSpawner, (asteroid_count, spawn_radius, seed, asteroid_bodies));