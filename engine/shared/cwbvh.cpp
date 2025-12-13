#include "cwbvh.hpp"

#include "bvh2.hpp"

#include "engine/core/renderer/voxel_object.hpp"

namespace tmt {

/* 8-wide BVH node. */
struct Bvh8Node {
    /* Minimum bounds */
    glm::vec3 min_bounds {};
    /* Left child node, or first primitive index */
    uint32_t first_prim = 0u;
    /* Maximum bounds */
    glm::vec3 max_bounds {};
    /* Primitive count */
    uint32_t prim_count = 0u;
    /* Child nodes */
    uint32_t children[8] {};
    uint32_t child_count = 0u;

    /* Returns true if this node is a leaf node. */
    bool is_leaf() const { return prim_count > 0; }
    /* Returns the surface area of this node. */
    float surface_area() const {
        const float extent_x = max_bounds.x - min_bounds.x;
        const float extent_y = max_bounds.y - min_bounds.y;
        const float extent_z = max_bounds.z - min_bounds.z;
        return extent_x * extent_y + extent_y * extent_z + extent_z * extent_x;
    }
};

template <typename T>
void CwBvh<T>::convert(const Bvh2<T>& bvh2) {
    /* Create a temporary BVH8 */
    Bvh8Node* bvh8_nodes = new Bvh8Node[std::max(2u, bvh2.node_count)] {};

    /* Mirror the BVH2 nodes */
    for (uint32_t i = 0u; i < bvh2.node_count; i++) {
        if (i == 1u) continue; /* Skip node number 1, it is invalid. */

        Bvh2Node& orig = bvh2.nodes[i];
        Bvh8Node& node = bvh8_nodes[i];
        node.min_bounds = orig.min_bounds, node.max_bounds = orig.max_bounds;
        if (orig.is_leaf())
            node.prim_count = orig.prim_count, node.first_prim = orig.left_first;
        else
            node.children[0] = orig.left_first, node.children[1] = orig.left_first + 1u, node.child_count = 2u;
    }

    { /* Collapse the BVH to use all 8 children */
        uint32_t stack[128] {};
        uint32_t stack_ptr = 0u, node_index = 0u;
        for (;;) {
            Bvh8Node& node = bvh8_nodes[node_index];

            for (; node.child_count < 8u;) {
                /* Try to find a child to collapse */
                uint32_t best_child = 256u;
                float best_child_sa = 0.0f;
                for (uint32_t i = 0u; i < node.child_count; ++i) {
                    const Bvh8Node& child = bvh8_nodes[node.children[i]];
                    if ((child.is_leaf() == false) && (node.child_count - 1u + child.child_count) <= 8u) {
                        const float surface_area = child.surface_area();
                        if (surface_area > best_child_sa) best_child = i, best_child_sa = surface_area;
                    }
                }

                /* Did not find a suitable child to collapse */
                if (best_child == 256u) break;

                /* Collapse the child node */
                const Bvh8Node& child = bvh8_nodes[node.children[best_child]];
                for (uint32_t i = 1u; i < child.child_count; ++i) {
                    node.children[node.child_count++] = child.children[i];
                }
                node.children[best_child] = child.children[0];
            }

            /* After collapsing this node, continue with its children */
            for (uint32_t i = 0u; i < node.child_count; ++i) {
                const uint32_t child_index = node.children[i];
                const Bvh8Node& child = bvh8_nodes[child_index];
                if (child.is_leaf() == false) stack[stack_ptr++] = child_index;
            }

            /* Traverse down the stack */
            if (stack_ptr == 0u) break;
            node_index = stack[--stack_ptr];
        }
    }

    /* Handle the special case where the root node is a leaf node */
    Bvh8Node& root_node = bvh8_nodes[0];
    if (root_node.is_leaf()) {
        bvh8_nodes[1] = root_node;
        root_node.child_count = 1u;
        root_node.children[0] = 1u;
        root_node.prim_count = 0u;
    }

    /* Allocate space for CWBVH nodes */
    if (node_count > 0u) {
        delete[] nodes;
        delete[] indices;
        node_count = 0u;
    }
    nodes = new CwBvhNode[std::max(2u, bvh2.node_count)];
    indices = new uint32_t[bvh2.prim_count];

    /* Compress the BVH8 into the CWBVH format */
    /* <https://research.nvidia.com/sites/default/files/publications/ylitie2017hpg-paper.pdf> */
    uint32_t src_stack[256] {}, dst_stack[256] {};
    uint32_t stack_ptr = 1u, node_ptr = 1u, prim_ptr = 0u;
    for (; stack_ptr > 0u;) {
        /* Get the source and destination nodes */
        const Bvh8Node& src = bvh8_nodes[src_stack[--stack_ptr]];
        CwBvhNode& dst = nodes[dst_stack[stack_ptr]];
        const glm::vec3 node_center = (src.min_bounds + src.max_bounds) * 0.5f;

        /* Octant-based child ordering for traversal coherence */
        /* Calculate edge weights for bipartite matching */
        float edge_w[8][8] {};
        uint32_t assignment[8] {};
        bool slot_empty[8] {};
        for (uint32_t s = 0u; s < 8u; ++s) {
            assignment[s] = 256u;
            slot_empty[s] = true;

            const glm::vec3 octant_dir((((s >> 2u) & 1u) == 1u) ? -1.0f : 1.0f, (((s >> 1u) & 1u) == 1u) ? -1.0f : 1.0f, (((s >> 0u) & 1u) == 1u) ? -1.0f : 1.0f);
            for (uint32_t i = 0u; i < 8u; ++i) {
                if (src.children[i] == 0u) {
                    edge_w[s][i] = BIG_F32;
                    continue;
                }

                /* Calculate the cost for this child/octant combination */
                const Bvh8Node& child = bvh8_nodes[src.children[i]];
                const glm::vec3 child_center = (child.min_bounds + child.max_bounds) * 0.5f;
                edge_w[s][i] = dot(child_center - node_center, octant_dir);
            }
        }

        /* Greedy bipartite matching, assigning children to octants */
        for (;;) {
            float min_cost = BIG_F32;
            uint32_t min_entry_x = 256u, min_entry_y = 256u;
            for (uint32_t s = 0u; s < 8u; ++s) {
                for (uint32_t i = 0u; i < 8u; ++i) {
                    if (assignment[i] == 256u && slot_empty[s] && edge_w[s][i] < min_cost) {
                        min_cost = edge_w[s][i], min_entry_x = s, min_entry_y = i;
                    }
                }
            }
            if (min_entry_x == 256u && min_entry_y == 256u) break;
            slot_empty[min_entry_x] = false, assignment[min_entry_y] = min_entry_x;
        }

        /* Fill in remaining slots */
        for (uint32_t i = 0u; i < 8u; ++i) {
            if (assignment[i] != 256u) continue;
            for (uint32_t s = 0u; s < 8u; ++s) {
                if (slot_empty[s] == false) continue;
                slot_empty[s] = false, assignment[i] = s;
                break;
            }
        }

        /* Assign the children to their new slots */
        uint32_t children[8] {};
        for (uint32_t i = 0u; i < 8u; ++i) children[assignment[i]] = src.children[i];

        /* Child bounds quantization parameters */
        dst.log_extent_x = (int8_t)ceilf(log2f((src.max_bounds.x - src.min_bounds.x) * (1.0f / 255.0f)));
        dst.log_extent_y = (int8_t)ceilf(log2f((src.max_bounds.y - src.min_bounds.y) * (1.0f / 255.0f)));
        dst.log_extent_z = (int8_t)ceilf(log2f((src.max_bounds.z - src.min_bounds.z) * (1.0f / 255.0f)));
        const float rq_x = 1.0f / exp2f((float)dst.log_extent_x);
        const float rq_y = 1.0f / exp2f((float)dst.log_extent_y);
        const float rq_z = 1.0f / exp2f((float)dst.log_extent_z);
        dst.min_bounds = src.min_bounds;

        /* Quantize the child nodes */
        uint32_t leaf_prim_count = 0u;
        dst.child_base_index = 0xFFFFFFFFu;
        dst.prim_base_index = 0xFFFFFFFFu;
        dst.intersection_mask = 0u;
        for (uint32_t i = 0u; i < 8u; ++i) {
            /* Clear empty slots */
            if (children[i] == 0u) {
                dst.meta[i] = 0u;
                dst.lo_x[i] = dst.lo_y[i] = dst.lo_z[i] = 0u;
                dst.hi_x[i] = dst.hi_y[i] = dst.hi_z[i] = 0u;
                continue;
            }

            /* Calculate quantized bounding box */
            Bvh8Node& child = bvh8_nodes[children[i]];
            dst.lo_x[i] = (uint8_t)floorf((child.min_bounds.x - src.min_bounds.x) * rq_x);
            dst.lo_y[i] = (uint8_t)floorf((child.min_bounds.y - src.min_bounds.y) * rq_y);
            dst.lo_z[i] = (uint8_t)floorf((child.min_bounds.z - src.min_bounds.z) * rq_z);
            dst.hi_x[i] = (uint8_t)ceilf((child.max_bounds.x - src.min_bounds.x) * rq_x);
            dst.hi_y[i] = (uint8_t)ceilf((child.max_bounds.y - src.min_bounds.y) * rq_y);
            dst.hi_z[i] = (uint8_t)ceilf((child.max_bounds.z - src.min_bounds.z) * rq_z);

            // if (child.is_leaf()) {
            //     printf("leaf:\n");
            //     printf("lo = %d, %d, %d\n", dst.lo_x[i], dst.lo_y[i], dst.lo_z[i]);
            //     printf("hi = %d, %d, %d\n", dst.hi_x[i], dst.hi_y[i], dst.hi_z[i]);
            // } else {
            //     printf("node:\n");
            //     printf("lo = %d, %d, %d\n", dst.lo_x[i], dst.lo_y[i], dst.lo_z[i]);
            //     printf("hi = %d, %d, %d\n", dst.hi_x[i], dst.hi_y[i], dst.hi_z[i]);
            // }

            if (child.is_leaf()) {
                /* Unary encode primitive count */
                if (dst.prim_base_index == 0xFFFFFFFFu) dst.prim_base_index = prim_ptr;
                const uint32_t unaryEncodedTriCount = child.prim_count == 1u ? 0b001u : child.prim_count == 2u ? 0b011u : 0b111u;

                /* Set the child metadata */
                dst.meta[i] = (uint8_t)((unaryEncodedTriCount << 5u) | (leaf_prim_count & 0b11111u));

                /* Copy over primitive indices */
                leaf_prim_count += child.prim_count;
                for (uint32_t j = 0u; j < child.prim_count; ++j) {
                    indices[prim_ptr++] = bvh2.indices[child.first_prim + j];
                }
            } else {
                const uint32_t new_index = node_ptr++;
                if (dst.child_base_index == 0xFFFFFFFFu) dst.child_base_index = new_index;
                dst.intersection_mask |= (1u << i);

                /* Set the child metadata */
                dst.meta[i] = (1u << 5u) | (24u + (i & 0b11111u));

                /* Push the child onto the stack */
                src_stack[stack_ptr] = children[i];
                dst_stack[stack_ptr++] = new_index;
            }
        }
    }

    /* Get the CWBVH node count and realloc the node array */
    node_count = node_ptr;
    nodes = (CwBvhNode*)realloc(nodes, node_count * sizeof(CwBvhNode));

    /* Free the temporary BVH8 nodes */
    delete[] bvh8_nodes;
}

template <typename T>
CwBvh<T>::~CwBvh() {
    if (node_count > 0u) {
        delete[] nodes;
        delete[] indices;
        node_count = 0u;
    }
}

/* Explicit template instantiations */
template class CwBvh<VoxelObject>;

}  // namespace tmt
