#pragma once
#include "core/system.hpp"

namespace tmt {

class Plan : public ISystem {
   public:
    Plan() = default;
    // Inherited via ISystem
    std::string get_name() override { return "Plan System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;
};

}  // namespace tmt
