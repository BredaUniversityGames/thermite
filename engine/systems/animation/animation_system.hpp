#pragma once

#include <cstdint>
#include "engine/core/ecs.hpp"

namespace tmt {

class RigModelManager : public ISystem {
   public:
    std::string get_name() override { return "animation system"; }

    void on_start() override {}
    void on_update(const FrameData& time) override;
    void on_end() override {}
};

}  // namespace tmt
