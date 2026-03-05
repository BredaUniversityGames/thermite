#pragma once

#include <cstdint>
#include "engine/core/ecs.hpp"
#include "engine/events/game.hpp"

namespace tmt {

class RigModelManager : public ISystem, OnGameStart  {
   public:
    std::string get_name() override;

    void on_game_start() override;
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;

    static void inspect(float);
};

}  // namespace tmt
