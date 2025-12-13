#pragma once

namespace tmt {

template<typename T>
class Bvh2;

/* Compressed 8-wide BVH node. */
struct CwBvhNode {
    /* Minimum bounds of this node. */
    glm::vec3 min_bounds {};

    /* Logarithm of the extent of this node. */
    int8_t log_extent_x = 0;
    int8_t log_extent_y = 0;
    int8_t log_extent_z = 0;
    uint8_t intersection_mask = 0u;

    /* Index of the first child node. (children are stored contiguously) */
    uint32_t child_base_index = 0u;
    /* Index of the first primitive. (primitives are stored contiguously) */
    uint32_t prim_base_index = 0u;

    /* Child node metadata. */
    uint8_t meta[8] {};
    /* Quantized child node bounding boxes. (40 bytes) */
    uint8_t lo_x[8] {};
    uint8_t lo_y[8] {};
    uint8_t lo_z[8] {};
    uint8_t hi_x[8] {};
    uint8_t hi_y[8] {};
    uint8_t hi_z[8] {};
};

static_assert(sizeof(CwBvhNode) == 80);

/*/
 * NOTE:
 * All BVH primitive types must implement the following functions:
 * 1. Aabb aabb() const;
 * 2. float intersect(const Ray&) const;
 * These are required for the BVH to function.
/*/

/* 8-wide Compressed Bounding Volume Hierarchy. */
template <typename T>
class CwBvh {
   public:
    /* Nodes & primitive indices */
    CwBvhNode* nodes = nullptr;
    uint32_t* indices = nullptr;
    uint32_t node_count = 0u;

    CwBvh() = default;
    ~CwBvh();

    /* Convert a 2-wide BVH into a CWBVH. */
    void convert(const Bvh2<T>& bvh2);
};

}  // namespace tmt
