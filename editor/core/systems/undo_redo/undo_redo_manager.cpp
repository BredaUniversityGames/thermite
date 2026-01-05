#include "undo_redo_manager.hpp"

#include <imgui.h>

#include "engine/core/logger.hpp"
#include "editor/editor.hpp"

namespace tmt {

UndoRedoManager& IUndoRedo::get_manager() { return editor.windows.get<UndoRedoManager>(); }

void UndoRedoManager::commit_action(const std::shared_ptr<IUndoRedo>& action, const std::string& message) {
    CommitAction entry {message, action};
    undo_stack.push_back(entry);
    redo_stack.clear();
}

void UndoRedoManager::display() {
    if (ImGui::Button("Undo")) {
        undo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) {
        redo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        undo_stack.clear();
        redo_stack.clear();
    }
    ImGui::BeginChild("UndoRedoManager_UndoStack", ImVec2(0, 200), true);
    ImGui::Text("Undo Stack:");
    for (int i = static_cast<int>(undo_stack.size()) - 1; i >= 0; i--) {
        const auto& entry = undo_stack[i];
        ImGui::BulletText("%s", entry.message.c_str());
    }
    ImGui::EndChild();
    ImGui::BeginChild("UndoRedoManager_RedoStack", ImVec2(0, 200), true);
    ImGui::Text("Redo Stack:");
    for (int i = static_cast<int>(redo_stack.size()) - 1; i >= 0; i--) {
        const auto& entry = redo_stack[i];
        ImGui::BulletText("%s", entry.message.c_str());
    }
    ImGui::EndChild();
}

void UndoRedoManager::on_editor_start() {}

void UndoRedoManager::on_editor_update(const FrameData& time) {}

void UndoRedoManager::on_editor_end() {}

void UndoRedoManager::undo() {
    if (undo_stack.empty()) {
        Log::warn(Log::Scope::EDITOR, "Undo stack is empty");
        return;
    }

    auto entry = std::move(undo_stack.back());
    undo_stack.pop_back();

    entry.action->undo();
    redo_stack.push_back(std::move(entry));
}
void UndoRedoManager::redo() {
    if (redo_stack.empty()) {
        Log::warn(Log::Scope::EDITOR, "Redo stack is empty");
        return;
    }

    auto entry = std::move(redo_stack.back());
    redo_stack.pop_back();

    entry.action->redo();
    undo_stack.push_back(std::move(entry));
}
}  // namespace tmt
