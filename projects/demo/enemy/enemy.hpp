#include "engine/core/ecs.hpp"

class Enemy {
   public:
    Enemy() {}
    ~Enemy() {}

    tmt::Entity asteroid_entity;

    void follow_path();
    void patrol_area();
    void chase_player();
    void attack_player();
};