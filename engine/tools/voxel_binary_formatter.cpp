#include "voxel_binary_formatter.hpp"

#include "core/resources/voxel_scene.hpp"

namespace tmt {

namespace {

// Helper structs with correct size to easily parse the binary data.
struct FileHeader {
    uint32_t magic_number;
    uint32_t major_version;
    uint32_t minor_version;
};
static_assert(sizeof(FileHeader) == 12);

struct HierarchyHeader {
    uint32_t node_count;
    uint32_t char_count;
};
static_assert(sizeof(HierarchyHeader) == 8);

struct HierarchyNode {
    uint32_t parent_index;
    uint32_t svt64_offset;
    uint32_t svt64_size;
    uint32_t name_offset;
    uint32_t name_length;
    glm::uvec3 size;
    glm::mat4 transform;
};
static_assert(sizeof(HierarchyNode) == 96);

struct SVT64Header {
    uint32_t node_count;
    uint32_t voxel_count;
    uint32_t depth;
};
static_assert(sizeof(SVT64Header) == 12);

template <typename Type>
const Type& read_data(const uint8_t* const& data_pointer) {
    const Type* value = reinterpret_cast<const Type*>(data_pointer);
    data_pointer += sizeof(Type);

    return *value;
}

template <typename Type>
std::span<const Type> read_data(const uint8_t* const& data_pointer, const uint32_t size) {
    const Type* data_start = reinterpret_cast<const Type*>(data_pointer);
    data_pointer += size * sizeof(Type);

    return {data_start, data_start + size};
}

template <typename Type>
void write_data(std::vector<uint8_t>& data, Type& value) {
    const size_t write_index = data.size();
    data.resize(write_index + sizeof(Type));
    std::memcpy(data.data() + write_index, &value, sizeof(Type));
}

template <typename Type>
void write_data(std::vector<uint8_t>& data, std::vector<Type>& value) {
    const size_t write_index = data.size();
    const size_t total_data_size = value.size() * sizeof(Type);

    data.resize(write_index + total_data_size);
    std::memcpy(data.data() + write_index, value.data(), total_data_size);
}

template <typename Type>
void write_data(std::vector<uint8_t>& data, const Type* write_data, const uint32_t write_data_count) {
    const size_t write_index = data.size();

    const size_t write_data_size = write_data_count * sizeof(Type);
    data.resize(write_index + write_data_size);

    std::memcpy(data.data() + write_index, write_data, write_data_size);
}

std::unique_ptr<Svt64> deserialize_svt64_tree(const std::span<const uint8_t>& svt64_data) {
    const uint8_t* data_pointer = svt64_data.data();
    const auto& header = read_data<SVT64Header>(data_pointer);

    auto tree = std::make_unique<Svt64>();
    tree->node_count = header.node_count;
    tree->voxel_count = header.voxel_count;
    tree->depth = header.depth;

    tree->palette = read_data<MaterialPalette>(data_pointer);

    tree->nodes = new Svt64Node[tree->node_count];
    const uint32_t nodes_data_size = tree->node_count * sizeof(Svt64Node);
    std::memcpy(tree->nodes, data_pointer, nodes_data_size);
    data_pointer += nodes_data_size;

    tree->materials = new MaterialIndex[tree->voxel_count + SVT64_BUFFER_MEMORY];
    const uint32_t material_data_size = tree->voxel_count * sizeof(MaterialIndex);
    std::memcpy(tree->materials, data_pointer, material_data_size);
    data_pointer += material_data_size;

    tree->physics_data = new PhysicsVoxel[tree->voxel_count + SVT64_BUFFER_MEMORY];
    const uint32_t physics_data_size = tree->voxel_count * sizeof(PhysicsVoxel);
    std::memcpy(tree->physics_data, data_pointer, physics_data_size);
    data_pointer += physics_data_size;

    return tree;
}

struct VoxelSceneDeserializeData {
    std::span<const HierarchyNode> hierarchy_nodes;
    std::string_view string_data;
    std::span<const uint8_t> svt64_data;
};

void recurse_deserialize_scene_node(VoxelSceneNode& scene_node, VoxelSceneDeserializeData& deserialize_data, size_t& node_index) {
    const HierarchyNode& node_data = deserialize_data.hierarchy_nodes[node_index];

    const auto& node_svt64_data = deserialize_data.svt64_data.subspan(node_data.svt64_offset, node_data.svt64_size);
    scene_node.tree = deserialize_svt64_tree(node_svt64_data);
    scene_node.size = node_data.size;
    scene_node.name = deserialize_data.string_data.substr(node_data.name_offset, node_data.name_length);
    scene_node.transform = node_data.transform;

    const size_t current_index = node_index;
    while ((node_index + 1) < deserialize_data.hierarchy_nodes.size() && deserialize_data.hierarchy_nodes[node_index + 1].parent_index == current_index) {
        recurse_deserialize_scene_node(scene_node.children.emplace_back(), deserialize_data, ++node_index);
    }
}

uint32_t serialize_svt64_tree(const Svt64& tree, std::vector<uint8_t>& svt64_data) {
    const size_t start_size = svt64_data.size();

    const SVT64Header header {
        .node_count = tree.node_count,
        .voxel_count = tree.voxel_count,
        .depth = tree.depth,
    };
    write_data(svt64_data, header);

    write_data(svt64_data, tree.palette);
    write_data(svt64_data, tree.nodes, tree.node_count);
    write_data(svt64_data, tree.materials, tree.voxel_count);
    write_data(svt64_data, tree.physics_data, tree.voxel_count);

    return static_cast<uint32_t>(svt64_data.size() - start_size);
}

struct VoxelSceneSerializeData {
    std::vector<HierarchyNode> hierarchy_nodes;
    std::string string_data;
    std::vector<uint8_t> svt64_data;
};

void recurse_parse_voxel_scene(const VoxelSceneNode& node, const uint32_t parent_index, VoxelSceneSerializeData& serialize_data) {
    const uint32_t current_index = static_cast<int32_t>(serialize_data.hierarchy_nodes.size());

    const uint32_t svt64_offset = static_cast<uint32_t>(serialize_data.svt64_data.size());
    const uint32_t svt64_size = serialize_svt64_tree(*node.tree, serialize_data.svt64_data);

    const uint32_t string_offset = static_cast<uint32_t>(serialize_data.string_data.size());
    serialize_data.string_data += node.name;

    const HierarchyNode hierarchy_node {
        .parent_index = parent_index,
        .svt64_offset = svt64_offset,
        .svt64_size = svt64_size,
        .name_offset = string_offset,
        .name_length = static_cast<uint32_t>(node.name.size()),
        .size = node.size,
        .transform = node.transform
    };
    serialize_data.hierarchy_nodes.push_back(hierarchy_node);

    for (const VoxelSceneNode& child : node.children) {
        recurse_parse_voxel_scene(child, current_index, serialize_data);
    }
}

}  // namespace

VoxelSceneNode load_voxel_scene(const std::vector<uint8_t>& data) {
    const uint8_t* data_pointer = data.data();

    // TODO: Actually handle the file header.
    [[maybe_unused]] const auto& file_header = read_data<FileHeader>(data_pointer);
    const auto& hierarchy_header = read_data<HierarchyHeader>(data_pointer);

    VoxelSceneDeserializeData deserialize_data;
    deserialize_data.hierarchy_nodes = read_data<HierarchyNode>(data_pointer, hierarchy_header.node_count);
    deserialize_data.string_data = std::string_view {reinterpret_cast<const char*>(data_pointer), hierarchy_header.char_count};
    data_pointer += hierarchy_header.char_count;
    deserialize_data.svt64_data = std::span<const uint8_t> {data_pointer, data.data() + data.size()};  // The rest of the file is svt64_data.

    size_t node_index = 0;
    VoxelSceneNode scene {};
    recurse_deserialize_scene_node(scene, deserialize_data, node_index);

    return scene;
}

std::vector<uint8_t> save_voxel_scene(const ResourceRef<VoxelScene>& scene) {
    std::vector<uint8_t> data {};

    // TODO: Actually handle the file header.
    FileHeader file_header {};
    write_data(data, file_header);

    VoxelSceneSerializeData serialize_data;
    recurse_parse_voxel_scene(scene->hierarchy, UINT_MAX, serialize_data);

    HierarchyHeader hierarchy_header {.node_count = static_cast<uint32_t>(serialize_data.hierarchy_nodes.size())};
    write_data(data, hierarchy_header);

    write_data(data, serialize_data.hierarchy_nodes);

    const uint8_t* string_data = reinterpret_cast<const uint8_t*>(serialize_data.string_data.data());
    data.insert(data.end(), string_data, string_data + serialize_data.string_data.size());

    // Insert the svt64 tree data at the end.
    data.insert(data.end(), serialize_data.svt64_data.begin(), serialize_data.svt64_data.end());

    return data;
}

}  // namespace tmt