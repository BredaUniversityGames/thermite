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

Entity recurse_create_entities(const VoxelSceneNode& node, const ResourceRef<VoxelScene>& scene) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(node.transform);

    if (node.tree) engine.ecs.add_component<VoxelRenderer>(entity).resource = engine.resources.copy_resource<VoxelVolume>(scene, node.uuid);

    for (const VoxelSceneNode& child : node.children) {
        const Entity child_entity = recurse_create_entities(child, scene);
        transform.add_child(child_entity);
    }

    return entity;
}

}  // namespace

void NodeHierarchy::menu_bar() {
    if (!ImGui::BeginMenuBar()) return;

    if (ImGui::MenuItem("Load"))
        open_file_dialog(
            [this](const IO::FileLocation& location) {
                model_load_atomic.store(false);

                const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
                recurse_create_entities(resource->hierarchy, resource);

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

        if (recurse_display_node(scene->hierarchy)) selected_scene = scene;
    }
}

bool NodeHierarchy::recurse_display_node(VoxelSceneNode& node) {
    const std::string uuid_string = node.uuid.str();
    ImGui::PushID(uuid_string.c_str());

    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (node.children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);

    const bool tree_open = ImGui::TreeNodeEx(node.name.c_str(), flags);
    bool is_clicked = ImGui::IsItemClicked();
    if (is_clicked) selected_node = &node;
    if (tree_open) {
        for (VoxelSceneNode& child : node.children) {
            is_clicked |= recurse_display_node(child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();

    return is_clicked;
}

}  // namespace tmt