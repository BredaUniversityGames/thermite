#include "edit.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/tools/svh_format.hpp"

#include "editor.hpp"
#include "editor/windows/node_hierarchy.hpp"

#include <imgui.h>

namespace tmt {

void EditMode::display_main_menu() {
    if (!ImGui::BeginMenu("File")) return;

    if (ImGui::MenuItem("New")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().new_svh();
    if (ImGui::MenuItem("Open...")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().open_svh();
    ImGui::Separator();
    if (ImGui::MenuItem("Save")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().save_svh();
    if (ImGui::MenuItem("Save As...")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().save_svh_as();
    ImGui::Separator();
    if (ImGui::MenuItem("Import...")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().import_file();
    if (ImGui::MenuItem("Export...")) editor.windows[Editor::Mode::EDIT].get<NodeHierarchy>().export_file();

    ImGui::EndMenu();
}

void EditMode::on_switch_to() {
    engine.scenes.load_scene<VoxelEditScene>();

    VoxelSceneNode node;
    if (!edit_data.empty() && decode_svh(edit_data, node)) NodeHierarchy::recurse_build_scene(node);

    edit_data.clear();
}

void EditMode::on_switch_away() { edit_data = NodeHierarchy::encode_voxel_scene(); }

}  // namespace tmt