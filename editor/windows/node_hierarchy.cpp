#include "node_hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"
#include "engine/tools/svh_format.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ogt_voxel_meshify.h>

#include <fstream>

namespace tmt {

namespace {

std::vector<uint8_t> create_uniform_voxels(const std::unique_ptr<Svt64>& tree, const glm::uvec3& size) {
    std::vector<uint8_t> voxels(size.x * size.y * size.z, 0);

    for (uint32_t x = 0; x < size.x; x++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t z = 0; z < size.z; z++) {
                const Material* material = tree->get_voxel(x, y, z);
                if (material == nullptr) continue;

                const uint8_t palette_index = static_cast<uint8_t>(material - tree->palette.entries);
                voxels[x + y * size.x + z * size.x * size.y] = palette_index + 1;  // Add one to account for 0 being air usually.
            }
        }
    }

    return voxels;
}

float linearize_srgb(const float value) {
    if (value <= 0.04045f) {
        return value / 12.92f;
    } else {
        return powf((value + 0.055f) / 1.055f, 2.4f);
    }
}

struct VoxelEditUUID {
    UUID uuid {NULL_UUID};
};

std::atomic_bool model_load_atomic {true};

}  // namespace

void NodeHierarchy::new_svh() {
    engine.ecs.clear();
    loaded_location = {};
}

void NodeHierarchy::open_svh() {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            loaded_location = location;
            engine.ecs.clear();

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            recurse_build_scene(resource->hierarchy);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{"Thermite voxel file", "svh"}}
    );
}

void NodeHierarchy::save_svh() {
    if (loaded_location.relative_path.empty()) {
        save_svh_as();
        return;
    }

    const std::vector<char> scene_data = encode_voxel_scene();
    IO::write_file(loaded_location, scene_data.data(), scene_data.size());
}

void NodeHierarchy::save_svh_as() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            loaded_location = location;

            const std::vector<char> scene_data = encode_voxel_scene();
            IO::write_file(location, scene_data.data(), scene_data.size());
        },
        {{"Thermite voxel file", "svh"}}
    );
}

void NodeHierarchy::import_file() {
    open_file_dialog(
        [this](const IO::FileLocation& location) {
            model_load_atomic.store(false);

            loaded_location = {};
            engine.ecs.clear();

            const ResourceRef<VoxelScene> resource = engine.resources.load_resource<VoxelScene>(location);
            recurse_build_scene(resource->hierarchy);

            model_load_atomic.store(true);
            model_load_atomic.notify_one();
        },
        {{"Vengi voxel file", "vengi"}}
    );
}

void NodeHierarchy::export_file() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            const auto& renderer_storage = engine.ecs.get_registry().storage<VoxelRenderer>();
            if (renderer_storage.empty()) return;

            const ResourceRef<VoxelVolume>& resource = renderer_storage.begin()->resource;
            const std::unique_ptr<Svt64>& tree = resource->blas;
            const glm::uvec3& size = resource->size;

            // Loop over all the voxels in the Svt64 tree to create a uniform voxel grid.
            const std::vector<uint8_t> uniform_voxels = create_uniform_voxels(tree, size);

            constexpr ogt_mesh_rgba palette[256] {};  // We don't really need to initialize the array, we only care about the indices, so we just need to allocate it for the ogt function.
            constexpr ogt_voxel_meshify_context context {};
            // Create a mesh from the uniform voxel grid.
            const ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_polygon(&context, uniform_voxels.data(), size.x, size.y, size.z, palette);

            const std::string obj_path = location.get_relative_path().generic_string();
            constexpr std::string_view mtl_filename {"palette.mtl"};
            const std::string mtl_path = location.get_relative_path().replace_filename(mtl_filename).generic_string();

            std::ofstream obj_file {obj_path, std::ios::trunc};
            if (obj_file.is_open()) {
                obj_file << std::format("mtllib {}", mtl_filename) << '\n';

                // Write all visuals to the file.
                for (uint32_t i = 0; i < mesh->vertex_count; i++) {
                    const ogt_mesh_vec3& coord = mesh->vertices[i].pos;
                    obj_file << std::format("v {} {} {}", coord.x, coord.y, coord.z) << '\n';
                }

                // Write all the polygons using the indices (+1 because .obj indices start at 1).
                for (uint32_t i = 0; i < mesh->index_count; i += 3) {
                    // Use the material with the correct color.
                    obj_file << std::format("usemtl {}", mesh->vertices[mesh->indices[i]].palette_index - 1) << '\n';

                    obj_file << std::format("f {} {} {}", mesh->indices[i + 0] + 1, mesh->indices[i + 1] + 1, mesh->indices[i + 2] + 1) << '\n';
                }

                obj_file.close();
            }

            std::ofstream mtl_file {mtl_path, std::ios::trunc};
            if (mtl_file.is_open()) {
                // Write the colors to the mtl file.
                for (uint32_t i = 0; i < 256; i++) {
                    // Start a new material.
                    mtl_file << std::format("newmtl {}", i) << '\n';

                    // Write the color then the alpha, which is always 1.0.
                    const Material& material = tree->palette.entries[i];
                    mtl_file << std::format("Kd {} {} {}", linearize_srgb(material.albedo_r), linearize_srgb(material.albedo_g), linearize_srgb(material.albedo_b)) << '\n';
                    mtl_file << "d 1.0" << '\n';
                }

                mtl_file.close();
            }
        },
        {{"Wavefront object model", "obj"}}
    );
}

void NodeHierarchy::display() {
    model_load_atomic.wait(false);

    const entt::basic_view view = engine.ecs.get_registry().view<Name, Transform>();
    for (const auto&& [entity, name, transform] : view.each()) {
        if (transform.get_parent() != entt::null) continue;

        recurse_display_node(entity, name, transform);
    }
}

Entity NodeHierarchy::recurse_build_scene(const VoxelSceneNode& node) {
    const Entity entity = engine.ecs.create_entity(node.name);

    Transform& transform = engine.ecs.get_component<Transform>(entity);
    transform.set_world_matrix(node.transform);

    VoxelEditUUID& uuid_component = engine.ecs.add_component<VoxelEditUUID>(entity);
    uuid_component.uuid = node.uuid;

    if (node.tree) {
        VoxelRenderer& renderer = engine.ecs.add_component<VoxelRenderer>(entity);

        // Create a fake voxel resource managed by the voxel editor.
        const ResourceRef volume {{}, std::make_shared<VoxelVolume>(node)};
        renderer.resource = volume;
    }

    for (const VoxelSceneNode& child : node.children) {
        const Entity child_entity = recurse_build_scene(child);
        transform.add_child(child_entity);
    }

    return entity;
}

void NodeHierarchy::recurse_parse_scene(Entity entity, const Transform& transform, VoxelSceneNode& node) {
    node.name = engine.ecs.get_component<Name>(entity).name;

    const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), transform.get_local_position());
    const glm::mat4 rotation = glm::toMat4(transform.get_local_rotation());
    const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), transform.get_local_scale());
    node.transform = translation * rotation * scale;

    node.uuid = engine.ecs.get_component<VoxelEditUUID>(entity).uuid;

    const VoxelRenderer* renderer = engine.ecs.try_get_component<VoxelRenderer>(entity);
    if (renderer != nullptr) {
        node.uuid = renderer->resource->uuid;
        node.size = renderer->resource->size;
        node.tree = std::make_unique<Svt64>(*renderer->resource->blas);
    }

    for (const Entity child : transform.get_children()) {
        const Transform& child_transform = engine.ecs.get_component<Transform>(child);
        recurse_parse_scene(child, child_transform, node.children.emplace_back());
    }
}

std::vector<char> NodeHierarchy::encode_voxel_scene() {
    const auto& entities = engine.ecs.get_registry().storage<Transform>();
    if (entities.empty()) return {};  // Return empty scene encoding.

    VoxelSceneNode node;
    for (const auto&& [entity, transform] : entities.each()) {
        if (transform.has_parent()) continue;

        recurse_parse_scene(entity, transform, node);
    }

    return encode_svh(node);
}

void NodeHierarchy::recurse_display_node(const Entity entity, const Name& name, const Transform& transform) {
    ImGui::PushID(static_cast<int>(entity));

    const std::set<Entity>& children = transform.get_children();

    constexpr ImGuiTreeNodeFlags default_flags =
        ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (children.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    flags |= (entity == selected_entity ? ImGuiTreeNodeFlags_Selected : 0);

    const bool tree_open = ImGui::TreeNodeEx(name.name.c_str(), flags);
    if (ImGui::IsItemClicked()) selected_entity = entity;
    if (tree_open) {
        for (const Entity child : children) {
            const auto&& [child_name, child_transform] = engine.ecs.get_component<Name, Transform>(child);
            recurse_display_node(child, child_name, child_transform);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

}  // namespace tmt