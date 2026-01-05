#include "goap_debugger.hpp"

#include <imgui.h>

#include "engine/engine.hpp"

#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"

#include <extern/imgui-node-editor/imgui_node_editor.h>
namespace ignode = ax::NodeEditor;
static ignode::EditorContext* g_Context = nullptr;  // internal state container for imgui-node-editor.

namespace tmt {

void GoapDebugger::on_editor_start() {
    if (!g_Context) {
        ignode::Config config;
        config.SettingsFile = "goap_graph.json";
        g_Context = ignode::CreateEditor(&config);
    }
}

void GoapDebugger::on_editor_end() {
    if (g_Context) {
        ignode::DestroyEditor(g_Context);
        g_Context = nullptr;
    }
}

void GoapDebugger::display() {
    auto& ecs = engine.ecs.get_registry();
    static Entity selected_agent = entt::null;

    // Gather all agents
    std::vector<Entity> agents;
    for (auto [e, agent] : ecs.view<GoapAgent>().each()) {
        agents.push_back(e);
    }

    ImGui::Begin("GOAP Debugger");

    // --- Agent Selector ---
    if (!agents.empty()) {
        static std::vector<std::string> agent_name_storage;
        agent_name_storage.clear();

        std::vector<const char*> agent_names;
        for (auto a : agents) {
            agent_name_storage.emplace_back("Agent " + std::to_string((uint32_t)a));
            agent_names.push_back(agent_name_storage.back().c_str());
        }

        static int current_index = 0;
        if (selected_agent != entt::null) {
            for (size_t i = 0; i < agents.size(); ++i) {
                if (agents[i] == selected_agent) current_index = (int)i;
            }
        }

        if (ImGui::Combo("Select Agent", &current_index, agent_names.data(), (int)agent_names.size())) {
            selected_agent = agents[current_index];
        }
    }

    // --- Draw only selected agent ---
    if (selected_agent != entt::null && ecs.any_of<GoapAgent, WorldState>(selected_agent)) {
        auto& agent = ecs.get<GoapAgent>(selected_agent);
        auto& ws = ecs.get<WorldState>(selected_agent);

        ImGui::BeginChild("Details", ImVec2(400, 0), true);
        draw_details_view(agent, ws);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Graph", ImVec2(0, 0), true);
        draw_goap_graph(agent, ws);
        ImGui::EndChild();
    }

    ImGui::End();
}

void GoapDebugger::draw_details_view(GoapAgent& agent, WorldState& ws) {
    // --- Active goal ---
    if (agent.has_goal()) {
        ImGui::Text("Active Goal: %s", agent.active_goal.name.c_str());
    } else {
        ImGui::Text("Active Goal: <none>");
    }

    // --- Plan ---
    if (ImGui::TreeNode("Plan")) {
        for (int i = 0; i < (int)agent.plan.size(); ++i) {
            bool current = (i == agent.current_index);
            ImGui::Text("%s %s", current ? "->" : " ", agent.plan[i]->get_id());
        }
        ImGui::TreePop();
    }

    // --- World state ---
    if (ImGui::TreeNode("World State")) {
        for (auto& [id, val] : ws.facts) {
            const std::string& fact_name = FactRegistry::instance().get_name(id);
            ImGui::PushID(id);  // ensure unique ImGui ID per fact

            switch (val.value_type) {
                case FactValue::Type::BOOL_TYPE: {
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

    // --- Available goals ---
    if (ImGui::TreeNode("Available Goals")) {
        for (auto& goal : agent.available_goals) {
            ImGui::Text("%s [priority: %d] %s", goal.name.c_str(), goal.priority, goal.is_relevant(ws) ? "(relevant)" : "(satisfied)");
        }
        ImGui::TreePop();
    }

    // --- Available actions ---
    if (ImGui::TreeNode("Available Actions")) {
        for (const GoapAction* action : agent.available_actions) {
            if (!action) continue;

            bool can_run = action->check_preconditions(ws);
            ImGui::Text("%s [cost: %.1f] %s", action->get_id(), action->cost, can_run ? "(can run)" : "(cannot run)");

            // --- Preconditions ---
            if (ImGui::TreeNode((std::string("Preconditions##") + std::to_string((uintptr_t)action)).c_str())) {
                for (auto& [key, val] : action->preconditions) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }

            // --- Effects ---
            if (ImGui::TreeNode((std::string("Effects##") + std::to_string((uintptr_t)action)).c_str())) {
                for (auto& [key, val] : action->effects) {
                    ImGui::Text("%s = %s", key.c_str(), val ? "true" : "false");
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    // --- Manual replan ---
    if (ImGui::Button("Force Replan")) {
        agent.needs_replan = true;
    }
}

struct PlanNode {
    int node;
    int inPin;
    int outPin;
};

void GoapDebugger::draw_goap_graph(GoapAgent& agent, WorldState& ws) {
    ignode::SetCurrentEditor(g_Context);
    ignode::Begin("GOAP Graph");

    int nodeId = 1000;
    int pinId = 2000;
    int linkId = 3000;

    const float planY = 0.0f;
    const float columnsY = 260.0f;

    const float colSpacing = 260.0f;
    const float rowSpacing = 260.0f;

    float planX = 0.0f;

    std::vector<PlanNode> planNodes;

    // ================= Plan =================
    for (int i = 0; i < (int)agent.plan.size(); ++i) {
        PlanNode n {nodeId++, pinId++, pinId++};
        planNodes.push_back(n);

        ignode::BeginNode(n.node);

        ImGui::Text("%s", agent.plan[i]->get_id());

        if (i == agent.current_index) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 50, 255));
            ImGui::Text("-> CURRENT");
            ImGui::PopStyleColor();
        }

        ignode::BeginPin(n.inPin, ignode::PinKind::Input);
        ignode::EndPin();
        ignode::BeginPin(n.outPin, ignode::PinKind::Output);
        ignode::EndPin();

        ignode::EndNode();
        ignode::SetNodePosition(n.node, ImVec2(planX + i * colSpacing, planY));
    }

    // ================= Active goal =================
    int goalNode = nodeId++;
    int goalPin = pinId++;

    ignode::BeginNode(goalNode);
    ImGui::Text("%s", agent.active_goal.name.c_str());
    ignode::BeginPin(goalPin, ignode::PinKind::Input);
    ignode::EndPin();
    ignode::EndNode();

    ignode::SetNodePosition(goalNode, ImVec2(planX + agent.plan.size() * colSpacing, planY));

    // ================= Plan links =================
    for (size_t i = 0; i + 1 < planNodes.size(); ++i) ignode::Link(linkId++, planNodes[i].outPin, planNodes[i + 1].inPin);

    if (!planNodes.empty()) ignode::Link(linkId++, planNodes.back().outPin, goalPin);

    // ================= Actions + goals rows =================
    float actionsX = 0.0f;
    float goalsX = actionsX + colSpacing;

    size_t rows = std::max(agent.available_actions.size(), agent.available_goals.size());

    for (size_t i = 0; i < rows; ++i) {
        float y = columnsY + i * rowSpacing;

        // ----- actions -----
        if (i < agent.available_actions.size()) {
            GoapAction* a = agent.available_actions[i];
            if (a) {
                int n = nodeId++;
                ignode::BeginNode(n);

                ImGui::Text("Action");
                ImGui::Separator();
                ImGui::Text("%s", a->get_id());
                ImGui::Text("Cost: %.1f", a->cost);

                ImGui::Text("Preconditions:");
                for (auto& [k, v] : a->preconditions) ImGui::BulletText("%s", k.c_str());

                ImGui::Text("Effects:");
                for (auto& [k, v] : a->effects) ImGui::BulletText("%s", k.c_str());
                ignode::EndNode();
                ignode::SetNodePosition(n, ImVec2(actionsX, y));
            }
        }

        // ----- Goals -----
        if (i < agent.available_goals.size()) {
            auto& g = agent.available_goals[i];
            int n = nodeId++;

            ignode::BeginNode(n);
            ImGui::Text("Goal");
            ImGui::Separator();
            ImGui::Text("%s", g.name.c_str());
            ImGui::Text("Priority: %d", g.priority);
            ImGui::Text("Relevance: %s", g.is_relevant(ws) ? "(relevant)" : "(satisfied)");
            ignode::EndNode();

            ignode::SetNodePosition(n, ImVec2(goalsX, y));
        }
    }

    // ================= Camera =================
    static bool firstFrame = true;
    if (firstFrame) {
        ignode::NavigateToContent();
        firstFrame = false;
    }

    ignode::End();
    ignode::SetCurrentEditor(nullptr);
}

}  // namespace tmt
