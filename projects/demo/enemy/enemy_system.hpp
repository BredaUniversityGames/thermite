#pragma once
#include <variant>
#include <cstdint>
#include "engine/core/ecs.hpp"
#include "enemy.hpp"

namespace tmt {

class EnemySystem : public ISystem {
   public:
    std::string get_name() override;

    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;
};
}  // namespace tmt