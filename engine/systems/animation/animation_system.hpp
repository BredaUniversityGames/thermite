#pragma once
#include <variant>
#include <cstdint>
#include "engine/core/ecs.hpp"
#include "rig_model.hpp"
#include "engine/systems/animation/components/constraints.hpp"

namespace tmt {

class RigModelManager : public ISystem {
   public:
    std::string get_name() override;

    void on_start() override;
    void on_update(const FrameData& time) override;
    void inspect(float);
    void on_end() override;
};

class ConstraintSystem : public ISystem 
{
   public:

// Inherited via ISystem
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
    std::string get_name() override;

   private:
};
}  // namespace tmt
