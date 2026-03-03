#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"

constexpr int leg_amount = 4;


class Walking : public tmt::GameComponent<Walking> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Walking"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float leg_time = 0.0f;
    float max_leg_time = 1.0f;
    tmt::Entity end_effector[leg_amount];
    tmt::Entity walkable_asteroid = entt::null;
    glm::vec3 velocity = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    float rotation_speed = 2.5f;
    float rotation_t = 0.0f;
    int ray_count = 100;   
    float height_above_ground = 0.2f;
    float walk_speed = 5.0f;
    float attack_range = 20.0f;
    float max_chase_distance = 100.0f;
};

TMT_OBJECT(Walking, (walkable_asteroid, rotation_speed, rotation_t, ray_count, height_above_ground, walk_speed, attack_range, max_chase_distance));