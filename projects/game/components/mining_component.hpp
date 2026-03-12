#pragma once
#include "debug_line_helper.hpp"
#include "events.hpp"
#include "ore_properties.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/shared/ray.hpp"

namespace game {

struct RayCylinder {
    // relative to the transform
    tmt::Entity base_transform_entity = entt::null;
    float base_radius = 0.20f;
    float ray_distance = 2.0f;
    uint32_t ray_amount = 20;
    float rotating_speed = 0.0f;
};
class MiningComponent : public tmt::GameComponent<MiningComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    void on_weapon_fired(const WeaponFiredEvent& e);

    bool active = true;
    DebugLineConfig cfg {};
    RayCylinder ray_cylinder {};
    // TODO replace with proper mask
    uint32_t ray_mask = 1 << 0;

   private:
    tmt::Transform* get_transform() const;
    void on_stop_mining(const ReleaseShootEvent& e);
    void handle_ore(const tmt::Hit& hit);
    std::vector<glm::vec3> compute_ray_origins(const tmt::Transform& transform) const;
    void assign_database();

    std::unordered_map<tmt::Material::Type, OreProperties::MiningOre>* ore_database { nullptr };

    std::vector<glm::vec3> previous_computed_origins;
    bool has_drawn_debug;             // for debug lines
    void mine(const glm::vec3& dir);  // base mining function, do not overload if using events, create new function instead
    struct PreviousAccumulatedHit {
        float mining_time = 0.0f;
        tmt::Material::Type type {};
        glm::uvec3 voxel_coord { std::numeric_limits<uint32_t>::max() };
        bool is_mining = false;
    };
    PreviousAccumulatedHit previous_hit;
};

}  // namespace game
TMT_OBJECT(game::RayCylinder, (base_transform_entity, base_radius, ray_distance, ray_amount, rotating_speed));
TMT_OBJECT(game::MiningComponent, (stencil, active, cfg, ray_cylinder, ray_mask));
