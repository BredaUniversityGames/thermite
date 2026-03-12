#pragma once
#include "engine/shared/ray.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

//custom hasher
namespace std {

template <>
struct hash<glm::uvec3> {
    size_t operator()(const glm::uvec3& v) const {
        size_t h1 = std::hash<uint32_t> {}(v.x);
        size_t h2 = std::hash<uint32_t> {}(v.y);
        size_t h3 = std::hash<uint32_t> {}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

}  // namespace std

namespace game {

class OreManager : public tmt::GameComponent<OreManager> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Ore Manager"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position);

    float thermite_ore_explosion_radius = 1.0f;
    float thermite_ore_explosion_damage = 10.0f;
    float thermite_ore_explosion_cooldown = 0.5f;

   private:
    std::unordered_set<glm::uvec3> positions_to_explode;
    std::unordered_set<glm::uvec3> new_positions_to_explode;
    std::unordered_set<tmt::Entity> hit_entities;
    float thermite_ore_explosion_cooldown_timer = 0.0f;
    void process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::uvec3 explosion_center);
};

}  
TMT_OBJECT(game::OreManager, (thermite_ore_explosion_radius, thermite_ore_explosion_damage, thermite_ore_explosion_cooldown));
