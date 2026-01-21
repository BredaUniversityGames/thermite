#include "node_hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"

#include <entt/entity/group.hpp>

namespace tmt {

namespace {

std::atomic_bool model_load_atomic {true};

void recurse_display_node(const VoxelSceneNode& node) {
    const std::string uuid_string = node.uuid.str();
    ImGui::PushID(uuid_string.c_str());

    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (node.children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);

    const bool tree_open = ImGui::TreeNodeEx(node.name.c_str(), flags);
    if (tree_open) {
        for (const VoxelSceneNode& child : node.children) {
            recurse_display_node(child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

}  // namespace

void NodeHierarchy::menu_bar() {
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::MenuItem("Load"))
        open_file_dialog(
            [this](const IO::FileLocation& location) {
                model_load_atomic.store(false);

                // const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
                // const ResourceRef<VoxelVolume> copy = engine.resources.copy_resource<VoxelVolume>(resource);
                model_load_atomic.store(true);
                model_load_atomic.notify_one();
            },
            {{"Voxel file", "svh;vengi"}}
        );

    ImGui::EndMenuBar();
}

void NodeHierarchy::display() {
    model_load_atomic.wait(false);
    menu_bar();

    const auto& scene_locations = engine.resources.get_resource_locations<VoxelScene>();
    for (const IO::FileLocation& file_location : scene_locations) {
        const auto scene = engine.resources.load_resource<VoxelScene>(file_location);

        recurse_display_node(scene->hierarchy);
    }
}

}  // namespace tmt