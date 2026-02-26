#pragma once
#include "engine/shared/svt64.hpp"

namespace tmt {

// 1024 ^3, max number of leaf nodes in a svt64 with the largest supported size
constexpr uint32_t MAX_VOXEL_NODES = 1073741824;

struct DestructionNode;

struct AxisConnections {
    uint8_t count = 0;
    DestructionNode* data[16];
};

struct DestructionNode {
    // Level is the inverse depth of the tree, where 0 is the voxel level
    uint8_t level = 0;

    // Index of the corresponding node in the svt64
    uint32_t tree_node = 0;

    glm::uvec3 global_position = glm::uvec3(0);

    // One vector per axis
    std::array<AxisConnections, 6> connections;
};

struct Destructable {
    uint32_t node_count = 0;
    std::unordered_map<uint64_t, DestructionNode*> nodes_map;
    std::vector<DestructionNode> nodes;
};

}  // namespace tmt