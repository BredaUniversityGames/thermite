#pragma once
#include "editor/core/window.hpp"
#include "../engine/systems/ai//goap/components/goap_action_overrides.hpp"

namespace tmt {

class GoapActionEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return "GOAP Actions"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;
};

}  // namespace tmt
