#include "svt64.hpp"

#include "immintrin.h"

namespace tmt {

/* Log with base. */
inline uint32_t log_base(const uint32_t x, const uint32_t b) { return (uint32_t)(log2((double)x) / log2((double)b)); }

Svt64::Node::Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask) {
    /* Only set the 31 least significant bits. */
    child_ptr = ptr & 0x7FFFFFFFu;
    child_mask = mask;

    /* Most significant bit is used to indicate a leaf node. */
    if (is_leaf) child_ptr |= 0x80000000u;
}

/* Recursive tree subdivide function. */
Svt64::Node Svt64::subdivide(const RawVoxels& raw_data, int scale, glm::ivec3 index) {
    /* Create a leaf node */
    if (scale == 2) {
        Node leaf_node = Node(true, voxel_count, 0x00);

        /* Check if the node is outside the voxel grid bounds */
        if (index.x + 3u >= raw_data.w || index.y + 3u >= raw_data.h || index.z + 3u >= raw_data.d) return leaf_node;
        if (index.x < 0 || index.y < 0 || index.z < 0) return leaf_node;

        const int wh = raw_data.h * raw_data.w;

        for (int i = 0; i < 64; ++i) {
            /* Fetch the voxel data */
            const int voxel_x = index.x + ((i >> 0) & 3);
            const int voxel_y = index.y + ((i >> 4) & 3);
            const int voxel_z = index.z + ((i >> 2) & 3);
            const int voxel_offset = voxel_z * wh + voxel_y * raw_data.w + voxel_x;
            const MaterialIndex material = raw_data.voxels[voxel_offset];

            if (material > 0u) {
                voxels[voxel_count++] = material;
                leaf_node.child_mask |= (1ull << i);
            }
        }

        return leaf_node;
    }

    /* Descend */
    scale -= 2;

    /* Collect child nodes */
    Node child_nodes[64] {};
    uint64_t child_mask = 0x00;
    uint32_t child_count = 0u;

    for (int i = 0; i < 64; ++i) {
        /* Subdivide the child node */
        const glm::ivec3 child_index = glm::ivec3(i >> 0 & 3, i >> 4 & 3, i >> 2 & 3);
        const Node child = subdivide(raw_data, scale, index + (child_index << scale));

        /* If the child is not empty */
        if (child.child_mask != 0u) {
            child_mask |= (1ull << i);
            child_nodes[child_count++] = child;
        }
    }

    /* Create a node */
    const Node node = Node(false, node_count, child_mask);

    /* Add child nodes into the nodes array */
    memcpy(nodes + node_count, child_nodes, child_count * sizeof(Node));
    node_count += child_count;
    return node;
}

void Svt64::build(const RawVoxels& raw_data) {
    /* Delete old data */
    if (nodes != nullptr) delete[] nodes;
    if (voxels != nullptr) delete[] voxels;

    /* Allocate space for new tree */
    nodes = (Node*)malloc(1ull << 26);
    node_count = 1u;
    voxels = (MaterialIndex*)malloc(1ull << 26);
    voxel_count = 0u;

    /* Required tree depth */
    req_depth = log_base(std::max(std::max(raw_data.w, raw_data.h), raw_data.d), 4);
    // printf("required depth: %u\n", req_depth);

    /* Begin the recursive build */
    nodes[0] = subdivide(raw_data, req_depth * 2, glm::ivec3(0));

    // printf("size: (%u, %u, %u), nodes: %u, voxels: %u\n", raw_data.w, raw_data.h, raw_data.d, node_count, voxel_count);

    // const uint64_t raw_size = raw_data.w * raw_data.h * raw_data.d;
    // const uint64_t tree_size = node_count * sizeof(Node) + voxel_count;
    // const float memory_percent = ((float)tree_size / (float)raw_size) * 100.0f;
    // printf("raw memory: %.2fMB, tree memory: %.2fMB (%.2f%%)\n", (float)raw_size / 1000000, (float)tree_size / 1000000, memory_percent);
}

inline uint32_t get_node_cell_index(const glm::vec3 pos, const int scale_exp) {
    const uint32_t cell_x = (uint32_t&)pos.x >> scale_exp & 3u;
    const uint32_t cell_y = (uint32_t&)pos.y >> scale_exp & 3u;
    const uint32_t cell_z = (uint32_t&)pos.z >> scale_exp & 3u;
    return cell_x + cell_y * 16u + cell_z * 4u;
}

inline glm::vec3 floor_scale(const glm::vec3 pos, const int scale_exp) {
    const uint32_t mask = ~0u << scale_exp;
    const uint32_t masked_x = (uint32_t&)pos.x & mask;
    const uint32_t masked_y = (uint32_t&)pos.y & mask;
    const uint32_t masked_z = (uint32_t&)pos.z & mask;
    return glm::vec3((float&)masked_x, (float&)masked_y, (float&)masked_z);
}

// Reverses `pos` from range [1.0, 2.0) to (2.0, 1.0] if `dir > 0`.
inline glm::vec3 mirror_pos(const glm::vec3 pos, const glm::vec3 dir) {
    glm::vec3 mirrored {};
    mirrored.x = dir.x > 0.0f ? (3.0f - pos.x) : pos.x;
    mirrored.y = dir.y > 0.0f ? (3.0f - pos.y) : pos.y;
    mirrored.z = dir.z > 0.0f ? (3.0f - pos.z) : pos.z;
    return mirrored;
}

// Count number of set bits in variable range [0..width]
inline uint32_t popcnt_var64(uint64_t mask, uint32_t width) { return __popcnt64(mask & ((1ull << width) - 1)); }

inline glm::vec2 intersect_aabb(const glm::vec3 origin, const glm::vec3 invDir, const glm::vec3 bbMin, const glm::vec3 bbMax) {
    glm::vec3 t0 = (bbMin - origin) * invDir;
    glm::vec3 t1 = (bbMax - origin) * invDir;

    const glm::vec3 temp = t0;
    t0 = glm::min(temp, t1), t1 = glm::max(temp, t1);

    const float tmin = glm::max(glm::max(glm::max(t0.x, t0.y), t0.z), 0.0f);
    const float tmax = glm::min(glm::min(t1.x, t1.y), t1.z);

    return glm::vec2(tmin, tmax);
}

inline float sign(const float f) { return f >= 0.0f ? 1.0f : -1.0f; }

uint64_t Svt64::memory_usage() const { return node_count * sizeof(Node) + voxel_count * sizeof(MaterialIndex); }

uint64_t Svt64::wasted_memory() const { return wasted_count; }

/* Credit: <https://dubiousconst282.github.io/2024/10/03/voxel-ray-tracing/> */
VoxelHit Svt64::trace(const Ray& ray) const {
    /* Traversal state */
    uint32_t stack[11] {};
    int scale_exp = 21;   /* 23 mantissa bits - 2 */
    uint32_t node_index = 0u; /* root node */
    Node node = nodes[node_index];

    const glm::vec2 hit = intersect_aabb(ray.origin, ray.rcp_dir, glm::vec3(1.0f), glm::vec3(2.0f));
    if (hit.y < hit.x) return VoxelHit(1e30f, glm::vec3(0.0f), 0u, 1u);

    const glm::vec3 origin = mirror_pos(ray.origin + ray.dir * hit.x, ray.dir);
    const glm::vec3 dir = ray.dir;

    /* Mirror coordinates to simplify cell intersections */
    uint32_t mirror_mask = 0x00;
    if (dir.x > 0.0f) mirror_mask |= 3u << 0;
    if (dir.y > 0.0f) mirror_mask |= 3u << 4;
    if (dir.z > 0.0f) mirror_mask |= 3u << 2;

    /* Safety clamp */
    glm::vec3 pos = clamp(origin, 1.0f, 1.9999999f);
    glm::vec3 inv_dir = 1.0f / -glm::abs(dir);

    glm::vec3 side_dist = glm::vec3(0.0f);
    int i = 0;

    for (i = 0; i < 256; i++) {
        uint32_t child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;

        /* Descend down the tree until we find an empty node or leaf node */
        while ((node.child_mask >> child_index & 1) != 0 && !node.is_leaf()) {
            /* Push the current node on the stack (at `scale_exp / 2`) */
            stack[scale_exp >> 1] = node_index;

            /* Fetch the child node */
            node_index = node.abs_ptr() + popcnt_var64(node.child_mask, child_index);
            node = nodes[node_index];

            /* Decrease the scale & get the next child index */
            scale_exp -= 2;
            child_index = get_node_cell_index(pos, scale_exp) ^ mirror_mask;
        }

        /* If this node is a leaf, check if we hit a voxel */
        if (node.is_leaf() && (node.child_mask >> child_index & 1) != 0) break;

        /* Check if we can actually take a larger step based on the child mask */
        int sub_scale_exp = scale_exp;
        if ((node.child_mask >> (child_index & 0b101010) & 0x00330033) == 0) sub_scale_exp++;

        // Compute next pos by intersecting with max cell sides
        const glm::vec3 cell_min = floor_scale(pos, sub_scale_exp);

        side_dist = (cell_min - origin) * inv_dir;
        float tmax = std::min(std::min(side_dist.x, side_dist.y), side_dist.z);

        const glm::ivec3 cell_min_i = glm::ivec3((int&)cell_min.x, (int&)cell_min.y, (int&)cell_min.z);

        glm::ivec3 neighbor_max = cell_min_i;
        neighbor_max.x += side_dist.x == tmax ? -1 : (1 << sub_scale_exp) - 1;
        neighbor_max.y += side_dist.y == tmax ? -1 : (1 << sub_scale_exp) - 1;
        neighbor_max.z += side_dist.z == tmax ? -1 : (1 << sub_scale_exp) - 1;

        /* Move to the entry point of our neighbour */
        pos = glm::min(origin - glm::abs(dir) * tmax, (glm::vec3&)neighbor_max);

        /* Find the first common ancestor node based on left-most carry bit */
        const glm::uvec3 diff_pos = glm::uvec3((uint32_t&)pos.x ^ (uint32_t&)cell_min.x, (uint32_t&)pos.y ^ (uint32_t&)cell_min.y, (uint32_t&)pos.z ^ (uint32_t&)cell_min.z);
        const int diff_exp = 31 - _lzcnt_u32((diff_pos.x | diff_pos.y | diff_pos.z) & 0xFFAAAAAA);

        /* Traverse back up the tree if we need to */
        if (diff_exp > scale_exp) {
            /* Break if we're exiting the root node */
            scale_exp = diff_exp;
            if (diff_exp > 21) break;

            /* Read the first common ancestor node from the stack */
            node_index = stack[scale_exp >> 1];
            node = nodes[node_index];
        }
    }

    /* If we ended in a leaf, we can gather the hit data we need */
    if (node.is_leaf() && scale_exp <= 21) {
        pos = mirror_pos(pos, dir);
        const float t = glm::length(pos - ray.origin);

        const uint32_t child_index = get_node_cell_index(pos, scale_exp);
        const MaterialIndex mat = voxels[node.abs_ptr() + popcnt_var64(node.child_mask, child_index)];

        // float tmax = min(min(sideDist.x, sideDist.y), sideDist.z);
        // bool3 sideMask = tmax >= sideDist;
        // hit.Normal = select(sideMask, -sign(dir), 0.0);
        // const float tmax = std::min(std::min(side_dist.x, side_dist.y), side_dist.z);
        glm::vec3 normal = glm::vec3(0.0f);
        if (side_dist.x < side_dist.y) {
            if (side_dist.x < side_dist.z) {
                normal.x = -sign(dir.x);
            } else {
                normal.z = -sign(dir.z);
            }
        } else {
            if (side_dist.y < side_dist.z) {
                normal.y = -sign(dir.y);
            } else {
                normal.z = -sign(dir.z);
            }
        }

        return VoxelHit(t, normal, mat, (uint16_t)i + 1u);
    }
    return VoxelHit(1e30f, glm::vec3(0.0f), 0u, (uint16_t)i + 1u);
}

void Svt64::set_voxel(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t node_index = 0u;
    uint32_t scale = req_depth * 2u;

    for (;;) {
        Node& node = nodes[node_index];

        if (node.is_leaf()) break;

        scale -= 2u;
        const uint32_t lx = (x >> scale) & 3u;
        const uint32_t ly = (y >> scale) & 3u;
        const uint32_t lz = (z >> scale) & 3u;
        const uint32_t li = lx + ly * 16u + lz * 4u;
        const uint64_t lm = 1ull << li;

        /* If the child doesn't exist yet, create it */
        if ((node.child_mask & lm) == 0ull) {
            const uint32_t prev_child_index = node.abs_ptr();
            wasted_count += popcnt_var64(node.child_mask, 64u) * sizeof(Node);
            node = Node(false, node_count, node.child_mask | lm);

            for (uint32_t i = 0u, j = 0u; i < 64u; ++i) {
                const bool is_new = i == li;

                if (is_new) {
                    /* Add new node */
                    nodes[node_count++] = Node(scale <= 2u, 0u, 0ull);
                } else {
                    /* Copy over old node */
                    if (node.child_mask & (1ull << i)) {
                        nodes[node_count++] = nodes[prev_child_index + j];
                        j++;
                    }
                }
            }
        }

        node_index = node.abs_ptr() + popcnt_var64(node.child_mask, li);
    }

    Node& node = nodes[node_index];

    const uint32_t lx = x & 3u;
    const uint32_t ly = y & 3u;
    const uint32_t lz = z & 3u;
    const uint32_t li = lx + ly * 16u + lz * 4u;
    const uint64_t lm = 1ull << li;

    /* If the voxel doesn't exist yet, create it */
    if ((node.child_mask & lm) == 0ull) {
        const uint32_t prev_voxel_index = node.abs_ptr();
        wasted_count += popcnt_var64(node.child_mask, 64u) * sizeof(MaterialIndex);

        MaterialIndex voxel {};

        node = Node(true, voxel_count, node.child_mask | lm);
        for (uint32_t i = 0u, j = 0u; i < 64u; ++i) {
            const bool is_new = i == li;

            if (is_new) {
                /* Add new voxel */
                voxels[voxel_count++] = voxel;
            } else {
                /* Copy over old voxel */
                if (node.child_mask & (1ull << i)) {
                    voxels[voxel_count++] = voxels[prev_voxel_index + j];
                    j++;
                }
            }
        }
    }
}

void Svt64::defrag() {
    /* Copy current nodes and voxels */
    Node* new_nodes = new Node[node_count * 2];
    MaterialIndex* new_voxels = new MaterialIndex[voxel_count * 2];

    /* Traverse the tree top down, move all child nodes/voxels into new lists */
    node_count = 1u;
    voxel_count = wasted_count = 0u;
    defrag_recurse(0u, 0u, new_nodes, new_voxels);

    delete[] nodes;
    nodes = new_nodes;
    delete[] voxels;
    voxels = new_voxels;
}

void Svt64::defrag_recurse(uint32_t old_index, uint32_t new_index, Node* new_nodes, MaterialIndex* new_voxels) {
#if 0 /* Recursive */
	const Node& old_node = nodes[old_index];
	Node& new_node = new_nodes[new_index];
	const uint32_t child_count = popcnt_var64(old_node.child_mask, 64u);
	const uint32_t child_ptr = old_node.abs_ptr();

	/* Defrag leaf node */
	if (old_node.is_leaf()) {
		new_node = Node(true, voxel_count, old_node.child_mask);
		memcpy(new_voxels + voxel_count, voxels + child_ptr, child_count * sizeof(MaterialIndex));
		voxel_count += child_count;
		return;
	}

	/* Defrag node */
	new_node = Node(false, node_count, old_node.child_mask);
	memcpy(new_nodes + node_count, nodes + child_ptr, child_count * sizeof(Node));
	node_count += child_count;

	/* Recurse into child nodes */
	for (uint32_t i = 0u; i < child_count; ++i) {
		defrag_recurse(child_ptr + i, new_node.abs_ptr() + i, new_nodes, new_voxels);
	}
#else /* Non-recursive */
    uint32_t old_stack[128] {};
    uint32_t new_stack[128] {};
    uint32_t stack_ptr = 0u;
    uint32_t old_node_index = 0u;
    uint32_t new_node_index = 0u;

    for (;;) {
        const Node& old_node = nodes[old_node_index];
        Node& new_node = new_nodes[new_node_index];
        const uint32_t child_count = popcnt_var64(old_node.child_mask, 64u);
        const uint32_t child_ptr = old_node.abs_ptr();

        /* Defrag leaf node */
        if (old_node.is_leaf()) {
            new_node = Node(true, voxel_count, old_node.child_mask);
            memcpy(new_voxels + voxel_count, voxels + child_ptr, child_count * sizeof(MaterialIndex));
            voxel_count += child_count;
            if (stack_ptr == 0u) break;
            old_node_index = old_stack[--stack_ptr];
            new_node_index = new_stack[stack_ptr];
            continue;
        }

        /* Defrag node */
        new_node = Node(false, node_count, old_node.child_mask);
        memcpy(new_nodes + node_count, nodes + child_ptr, child_count * sizeof(Node));
        node_count += child_count;

        /* Recurse into child nodes */
        for (uint32_t i = 0u; i < child_count; ++i) {
            old_stack[stack_ptr] = child_ptr + i;
            new_stack[stack_ptr++] = new_node.abs_ptr() + i;
        }

        if (stack_ptr == 0u) break;
        old_node_index = old_stack[--stack_ptr];
        new_node_index = new_stack[stack_ptr];
    }
#endif
}

Svt64::~Svt64() {
    if (nodes != nullptr) delete[] nodes;
    if (voxels != nullptr) delete[] voxels;
}

}  // namespace tmt
