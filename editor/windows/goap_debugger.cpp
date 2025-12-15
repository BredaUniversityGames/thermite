#include "goap_debugger.hpp"

#include <imgui.h>

#include "engine/engine.hpp"

#include "engine/systems/ai/components/goap_agent.hpp"
#include "engine/systems/ai/components/world_state.hpp"

namespace tmt {

void GoapDebugger::display() {
    auto& ecs = engine.ecs.get_registry();

    if (ImGui::Begin("GOAP Debugger")) {
        ecs.view<GoapAgent, WorldState>().each([&](Entity e, GoapAgent& agent, WorldState& ws) {
            ImGui::SeparatorText(("Agent " + std::to_string((uint32_t)e)).c_str());

            // Active goal
            if (agent.has_goal()) {
                ImGui::Text("Active Goal: %s", agent.active_goal.name.c_str());
            } else {
                ImGui::Text("Active Goal: <none>");
            }

            // Plan
            if (ImGui::TreeNode("Plan")) {
                for (int i = 0; i < (int)agent.plan.size(); ++i) {
                    bool current = (i == agent.current_index);
                    ImGui::Text("%s %s", current ? "->" : " ", agent.plan[i]->get_name());
                }
                ImGui::TreePop();
            }

            // World state
            if (ImGui::TreeNode("World State")) {
                for (auto& [id, val] : ws.facts) {
                    const std::string& fact_name = FactRegistry::instance().get_name(id);

                    ImGui::PushID(id);  // ensure unique ImGui ID per fact

                    switch (val.value_type) {
                        case FactValue::Type::BOOL_TYPE: {
                            bool old = val.bool_val;
                            if (ImGui::Checkbox(fact_name.c_str(), &val.bool_val)) {
                                agent.needs_replan = true;
                            }
                            break;
                        }

                        case FactValue::Type::INT_TYPE:
                            ImGui::Text("%s = %d", fact_name.c_str(), val.int_val);
                            break;

                        case FactValue::Type::FLOAT_TYPE:
                            ImGui::Text("%s = %.2f", fact_name.c_str(), val.float_val);
                            break;
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            // Manual replan
            if (ImGui::Button("Force Replan")) {
                agent.needs_replan = true;
            }
        });
    }
    ImGui::End();
}

}  // namespace tmt
