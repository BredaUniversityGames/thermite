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

namespace tmt {

namespace {

enum : uint8_t { OBJ_KD, OBJ_KA, OBJ_KE, OBJ_KS, OBJ_NS, OBJ_KN, OBJ_PROP_COUNT };

struct Vertex {
    glm::vec3 v;  // position?
    glm::vec3 t;  // tangent
    glm::vec3 n;  // normal
};

struct Polygon {
    int vi[3];  // vertex indices
};

struct Line {
    int vi[2];  // vertex indices
};

struct Surface {
    int mi {0};  // material index

    std::vector<Polygon> polygons;  // polygon vector
};

struct Property {
    const char* str {nullptr};
    glm::vec4 o;  // offset
    glm::vec4 s;  // scale
    glm::vec4 c;  // color
};

struct ObjectMaterial {
    std::string name;

    Property kv[OBJ_PROP_COUNT];  // material property vector?
};

struct obj {
    std::vector<Vertex> vertices;     // vertex vector
    std::vector<Surface> surfaces;    // surface vector
    std::vector<ObjectMaterial> materials;  // material vector
};

void obj_write_map(FILE* fout, const obj& O, int mi, int ki, const char* s) {
    const Property& kp = O.materials[mi].kv[ki];

    /* If this property has a map... */

    if (kp.str) {
        fprintf(fout, "map_%s ", s);

        /* Store all map options. */

        // if ((kp->opt & OBJ_OPT_CLAMP) != 0) fprintf(fout, "-clamp on ");

        /* Store the map offset, if any. */

        if (fabs(kp.o[0]) > 0 || fabs(kp.o[1]) > 0 || fabs(kp.o[2]) > 0) fprintf(fout, "-o %f %f %f ", kp.o[0], kp.o[1], kp.o[2]);

        /* Store the map scale, if any. */

        if (fabs(kp.s[0] - 1) > 0 || fabs(kp.s[1] - 1) > 0 || fabs(kp.s[2] - 1) > 0) fprintf(fout, "-s %f %f %f ", kp.s[0], kp.s[1], kp.s[2]);

        /* Store the map image file name. */

        fprintf(fout, "%s\n", kp.str);
    }
}

void obj_write_mtl(const obj& O, const char* mtl) {
    FILE* fout;
    int mi;

    if ((fout = fopen(mtl, "w"))) {
        for (mi = 0; mi < O.materials.size(); ++mi) {
            const ObjectMaterial& mp = O.materials[mi];

            /* Start a new material. */

            if (!mp.name.empty())
                fprintf(fout, "newmtl %s\n", mp.name.c_str());
            else
                fprintf(fout, "newmtl default\n");

            /* Store all material property colors. */

            fprintf(fout, "Kd %12.8f %12.8f %12.8f\n", mp.kv[OBJ_KD].c[0], mp.kv[OBJ_KD].c[1], mp.kv[OBJ_KD].c[2]);
            fprintf(fout, "d  %12.8f\n", mp.kv[OBJ_KD].c[3]);

            /* Store all material property maps. */

            obj_write_map(fout, O, mi, OBJ_KD, "Kd");
        }
    }
    fclose(fout);
}

void obj_write_obj(const obj& O, const char* obj, const char* mtl, int prec) {
    FILE* fout = fopen(obj, "w");

    if (fout == nullptr) return;
    char formv[256];
    //char formt[256];
    //char formn[256];

    size_t si;
    size_t vi;
    size_t pi;

    if (mtl) fprintf(fout, "mtllib %s\n", mtl);

    /* Store all vertex data. */

    sprintf(formv, "v %%.%df %%.%df %%.%df\n", prec, prec, prec);

    for (vi = 0; vi < O.vertices.size(); ++vi) fprintf(fout, formv, O.vertices[vi].v[0], O.vertices[vi].v[1], O.vertices[vi].v[2]);

    for (si = 0; si < O.surfaces.size(); ++si) {
        int mi = O.surfaces[si].mi;

        /* Store the surface's material reference */

        if (0 <= mi && mi < O.materials.size() && !O.materials[mi].name.empty())
            fprintf(fout, "usemtl %s\n", O.materials[O.surfaces[si].mi].name.c_str());
        else
            fprintf(fout, "usemtl default\n");

        /* Store all polygon definitions. */

        for (pi = 0; pi < O.surfaces[si].polygons.size(); pi++) {
            const int vi0 = O.surfaces[si].polygons[pi].vi[0] + 1;
            const int vi1 = O.surfaces[si].polygons[pi].vi[1] + 1;
            const int vi2 = O.surfaces[si].polygons[pi].vi[2] + 1;

            fprintf(fout, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", vi0, vi0, vi0, vi1, vi1, vi1, vi2, vi2, vi2);
        }
    }

    fclose(fout);
}

void obj_write(const obj& O, const char* obj, const char* mtl, int prec) {
    if (obj) obj_write_obj(O, obj, mtl, prec);
    if (mtl) obj_write_mtl(O, mtl);
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

std::vector<uint8_t> create_uniform_voxels(const std::unique_ptr<Svt64>& tree, const glm::uvec3& size) {
    std::vector<uint8_t> voxels(size.x * size.y * size.z, 0);

    for (uint32_t x = 0; x < size.x; x++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t z = 0; z < size.z; z++) {
                const Material* material = tree->get_voxel(x, y, z);
                if (material == nullptr) continue;

                const uint8_t palette_index = static_cast<uint8_t>(material - tree->palette.entries);
                voxels[x + y * size.x + z * size.x * size.y] = palette_index + 1; // Add one to account for 0 being air usually.
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

void NodeHierarchy::export_file() {
    save_file_dialog(
        [this](const IO::FileLocation& location) {
            const auto& renderer_storage = engine.ecs.get_registry().storage<VoxelRenderer>();
            if (renderer_storage.empty()) return;

            auto& resource = renderer_storage.begin()->resource;
            const std::unique_ptr<Svt64>& tree = resource->blas;
            const glm::uvec3& size = resource->size;

            const std::vector<uint8_t> uniform_voxels = create_uniform_voxels(tree, size);

            ogt_mesh_rgba palette[256];
            for (uint32_t i = 0; i < 256; i++) {
                palette[i].r = static_cast<uint8_t>(tree->palette.entries[i].albedo_r * 255);
                palette[i].g = static_cast<uint8_t>(tree->palette.entries[i].albedo_g * 255);
                palette[i].b = static_cast<uint8_t>(tree->palette.entries[i].albedo_b * 255);
                palette[i].a = 255;
            }

            constexpr ogt_voxel_meshify_context context {};
            const ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_polygon(&context, uniform_voxels.data(), size.x, size.y, size.z, palette);

            obj object {};
            object.vertices.resize(mesh->vertex_count);
            for (uint32_t i = 0; i < object.vertices.size(); i++) {
                object.vertices[i].v = glm::make_vec3(&mesh->vertices[i].pos.x);
                object.vertices[i].n = glm::make_vec3(&mesh->vertices[i].normal.x);
            }

            object.surfaces.resize(mesh->index_count / 3);
            uint32_t index_index = 0;
            for (Surface& surface : object.surfaces) {
                surface.mi = mesh->vertices[mesh->indices[index_index]].palette_index - 1;

                Polygon& polygon = surface.polygons.emplace_back();
                polygon.vi[0] = mesh->indices[index_index++];
                polygon.vi[1] = mesh->indices[index_index++];
                polygon.vi[2] = mesh->indices[index_index++];
            }

            object.materials.resize(256);
            for (uint32_t i = 0; i < object.materials.size(); i++) {
                object.materials[i].name = std::to_string(i);
                object.materials[i].kv[OBJ_KD].c[0] = linearize_srgb(tree->palette.entries[i].albedo_r);
                object.materials[i].kv[OBJ_KD].c[1] = linearize_srgb(tree->palette.entries[i].albedo_g);
                object.materials[i].kv[OBJ_KD].c[2] = linearize_srgb(tree->palette.entries[i].albedo_b);
                object.materials[i].kv[OBJ_KD].c[3] = 1.0f;
            }

            const std::string obj_path = location.get_relative_path().generic_string();
            const std::string mtl_path = location.get_relative_path().replace_extension(".mtl").generic_string();
            obj_write(object, obj_path.c_str(), mtl_path.c_str(), 4);
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