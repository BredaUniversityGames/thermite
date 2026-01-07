#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/tools/serializer/glm.hpp"

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float camera_sensitivity = 0.1f;

    glm::vec3 velocity = {0.0f, 0.0f, 0.0f};
    float acceleration = 20.0f;  // How fast you accelerate
    float drag = 5.0f;           // How fast you decelerate (higher = stops faster)
    float max_speed = 10.0f;     // Speed cap
};
TMT_OBJECT(Player, (camera_sensitivity, velocity, acceleration, drag, max_speed));