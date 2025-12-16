#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"

namespace tmt {

class GoapDebugger : public IWindow {
   public:
    constexpr std::string get_title() const override { return "GOAP Debugger"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;

   private:
    void draw_details_view(GoapAgent& agent, WorldState& ws);
    void draw_goap_graph(GoapAgent& agent, WorldState& ws);
};

}  // namespace tmt
