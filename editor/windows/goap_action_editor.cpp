#include "goap_action_editor.hpp"
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

#include <imgui.h>
#include <string>
#include <vector>

#include <extern/imgui-node-editor/imgui_node_editor.h>
namespace ignode = ax::NodeEditor;
static ignode::EditorContext* g_Context = nullptr;  // internal state container for imgui-node-editor.

namespace tmt {

void GoapActionEditor::on_editor_start() { GoapActionOverrides::instance().load(); }

void GoapActionEditor::on_editor_end() { GoapActionOverrides::instance().save(); }

void GoapActionEditor::display() {
    const auto& actions_map = GoapActionRegistry::instance().get_all();
    if (actions_map.empty()) {
        ImGui::Text("No actions registered.");
        return;
    }

    // --- Build selection list ---
    static int selected_index = 0;
    static std::vector<std::string> action_ids;
    static std::vector<const char*> action_labels;

    if (action_ids.empty()) {
        for (const auto& [id, action] : actions_map) {
            action_ids.push_back(id);
            action_labels.push_back(action_ids.back().c_str());
        }
    }

    ImGui::Combo("Select Action", &selected_index, action_labels.data(), static_cast<int>(action_labels.size()));

    std::string sel_id = action_ids[selected_index];
    GoapAction* action = GoapActionRegistry::instance().get(sel_id);
    if (!action) return;

    ImGui::SeparatorText("Cost");
    ImGui::DragFloat("Cost", &action->cost, 0.1f, 0.0f, 100.0f);

    // --- Preconditions ---
    if (ImGui::TreeNode("Preconditions")) {
        for (auto it = action->preconditions.begin(); it != action->preconditions.end();) {
            ImGui::PushID(it->first.c_str());
            ImGui::Checkbox(it->first.c_str(), &it->second);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                it = action->preconditions.erase(it);
            } else {
                ++it;
            }
            ImGui::PopID();
        }

        static char newFact[64] = "";
        ImGui::InputText("Add Fact", newFact, 64);
        ImGui::SameLine();
        if (ImGui::Button("Add##Precondition")) {
            if (strlen(newFact) > 0) {
                action->preconditions[newFact] = true;
                newFact[0] = '\0';
            }
        }

        ImGui::TreePop();
    }

    // --- Effects ---
    if (ImGui::TreeNode("Effects")) {
        for (auto it = action->effects.begin(); it != action->effects.end();) {
            ImGui::PushID(it->first.c_str());
            ImGui::Checkbox(it->first.c_str(), &it->second);
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                it = action->effects.erase(it);
            } else {
                ++it;
            }
            ImGui::PopID();
        }

        static char newEffect[64] = "";
        ImGui::InputText("Add Effect", newEffect, 64);
        ImGui::SameLine();
        if (ImGui::Button("Add##Effect")) {
            if (strlen(newEffect) > 0) {
                action->effects[newEffect] = true;
                newEffect[0] = '\0';
            }
        }

        ImGui::TreePop();
    }
}

}  // namespace tmt
