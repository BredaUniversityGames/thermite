#pragma once

#include <cstdint>
#include <vector>

#include "core/renderer/material.hpp"
#include "ray.hpp"

namespace tmt {

struct RawVoxels {
    MaterialPalette palette {};
    std::vector<uint8_t> voxels {};
    uint32_t w = 0u, h = 0u, d = 0u;
};

struct VoxelHit {
    float t = 1e30f;
    glm::vec3 normal = glm::vec3(0.0f);
    uint8_t material {};
    uint16_t steps = 0u;

    VoxelHit() = default;
    VoxelHit(float t, glm::vec3 normal, uint8_t mat, uint16_t steps) : t(t), normal(normal), material(mat), steps(steps) {};
};

/* 64-wide Sparse Voxel Tree. */
class Svt64 {
    struct Node {
        /* The most significant bit indicates if this node is a leaf containing voxels. */
        /* The 31 least significant bits are an absolute offset into an array of child nodes / voxels. */
        uint32_t child_ptr = 0u;
        /* Indicates which child nodes / voxels are present using 1 bit per child. */
        uint64_t child_mask = 0u;

        Node() = default;
        Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask);

        /* Is this node a leaf node? */
        inline bool is_leaf() const { return (child_ptr >> 31u) == 1u; };

        /* Absolute offset into an array of child nodes / voxels. */
        inline uint32_t abs_ptr() const { return child_ptr & 0x7FFFFFFFu; };
    };

    /* List of tree nodes. */
    Node* nodes = nullptr;
    uint32_t node_count = 0u;

    /* List of voxel data. */
    MaterialPalette palette {};
    MaterialIndex* voxels = nullptr;
    uint32_t voxel_count = 0u;
    uint32_t req_depth = 0u;

    /* Number of wasted bytes. */
    uint32_t wasted_count = 0u;

    /* Recursive tree subdivide function. */
    Node subdivide(const RawVoxels& raw_data, int scale, glm::ivec3 index);

   public:
    Svt64() = default;
    ~Svt64();

    /* Build the Sparse Voxel Tree. */
    void build(const RawVoxels& raw_data);

    /* Defragment the Tree. */
    void defrag();

    void defrag_recurse(uint32_t old_index, uint32_t new_index, Node* new_nodes, uint8_t* new_voxels);

    /* Get the current memory usage of the 64tree. */
    uint64_t memory_usage() const;
    /* Get the current wasted memory of the 64tree. */
    uint64_t wasted_memory() const;

    VoxelHit trace(const Ray& ray) const;

    void set_voxel(uint32_t x, uint32_t y, uint32_t z);
};

}  // namespace tmt
