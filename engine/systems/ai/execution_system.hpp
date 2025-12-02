#pragma once
#include "core/system.hpp"

namespace tmt {

class Execution : public ISystem {
   public:
    Execution() = default;
    // Inherited via ISystem
    std::string get_name() override { return "Action Execution System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;
};

}  // namespace tmt
