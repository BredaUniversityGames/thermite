#include "svt64.hpp"

#include <nmmintrin.h> /* popcnt64 */

#include "engine/tools/profiler.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

/* Log with base. */
inline uint32_t log_base(const uint32_t x, const uint32_t b) { return (uint32_t)ceil(log((double)x) / log((double)b)); }

/* Calculate the depth of a SVT64 based on its input voxel grid size. */
inline uint32_t tree_depth(uint32_t width, uint32_t height, uint32_t depth) {
    TMT_ZONE_SCOPED

    const float max_axis = (float)std::max(std::max(width, height), depth);
    return (uint32_t)ceilf(logf(max_axis) / logf(4.0f));
}

/* Calculate the maximum number of nodes a SVT64 can have given its depth. */
inline uint32_t max_node_count(uint32_t depth) {
    TMT_ZONE_SCOPED

    uint32_t node_count = 0u;
    for (int i = (int)depth - 1; i >= 0; --i) {
        const uint32_t width = (uint32_t)powf(4.0f, (float)i);
        node_count += width * width * width;
    }
    return node_count;
}

/* Find out how many solid voxels are inside of some raw voxel data. */
inline uint32_t raw_voxel_count(const RawVoxels& data) {
    TMT_ZONE_SCOPED

    uint32_t count = 0u;
    for (uint32_t z = 0u; z < data.d; ++z) {
        for (uint32_t y = 0u; y < data.h; ++y) {
            for (uint32_t x = 0u; x < data.w; ++x) {
                if (data.materials[z * data.w * data.h + y * data.w + x] != AIR_INDEX) count++;
            }
        }
    }
    return count;
}

Svt64Node::Svt64Node(const bool is_leaf, const uint32_t ptr, const uint64_t mask) {
    TMT_ZONE_SCOPED

    /* Only set the 31 least significant bits. */
    child_ptr = ptr & 0x7FFFFFFFu;
    child_mask = mask;

    /* Most significant bit is used to indicate a leaf node. */
    if (is_leaf) child_ptr |= 0x80000000u;
}

/* Recursive tree subdivide function. */
Svt64Node Svt64::subdivide(const RawVoxels& raw_data, uint32_t scale, glm::uvec3 index) {
    TMT_ZONE_SCOPED

    /* Create a leaf node */
    if (scale == 2u) {
        Svt64Node leaf_node = Svt64Node(true, voxel_count, 0x00);

        /* Check if the node is outside the voxel grid bounds */
        if (index.x >= raw_data.w || index.y >= raw_data.h || index.z >= raw_data.d) return leaf_node;

        const uint32_t wh = raw_data.h * raw_data.w;

        for (uint32_t i = 0u; i < 64u; ++i) {
            /* Fetch the voxel data */
            const uint32_t voxel_x = index.x + ((i >> 0u) & 3u);
            const uint32_t voxel_y = index.y + ((i >> 4u) & 3u);
            const uint32_t voxel_z = index.z + ((i >> 2u) & 3u);
            if (voxel_x >= raw_data.w || voxel_y >= raw_data.h || voxel_z >= raw_data.d) continue;
            const uint32_t voxel_offset = voxel_z * wh + voxel_y * raw_data.w + voxel_x;
            const MaterialIndex material = raw_data.materials[voxel_offset];

            if (material != AIR_INDEX) {
                materials[voxel_count] = material;
                physics_data[voxel_count++] = raw_data.physics_data[voxel_offset];
                leaf_node.child_mask |= (1ull << i);
            }
        }

        return leaf_node;
    }

    /* Descend */
    scale -= 2u;

    /* Collect child nodes */
    Svt64Node child_nodes[64] {};
    uint64_t child_mask = 0x00u;
    uint32_t child_count = 0u;

    for (uint32_t i = 0u; i < 64u; ++i) {
        /* Subdivide the child node */
        const glm::uvec3 child_index = glm::uvec3(i >> 0u & 3u, i >> 4u & 3u, i >> 2u & 3u);
        const Svt64Node child = subdivide(raw_data, scale, index + (child_index << scale));

        /* If the child is not empty */
        if (child.child_mask != 0u) {
            child_mask |= (1ull << i);
            child_nodes[child_count++] = child;
        }
    }

    /* Create a node */
    const Svt64Node node = Svt64Node(false, node_count, child_mask);

    /* Add child nodes into the nodes array */
    memcpy(nodes + node_count, child_nodes, child_count * sizeof(Svt64Node));
    node_count += child_count;
    return node;
}

bool Svt64::is_empty(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return true;
        } else if (level == depth) {
            return false;
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return true;
}

Material* Svt64::get_voxel(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return nullptr;
        } else if (level == depth) {
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &palette.entries[materials[current->abs_ptr() + child_pos]];
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return nullptr;
}

PhysicsVoxel* Svt64::get_physics_voxel(const uint32_t x, const uint32_t y, const uint32_t z) {
    TMT_ZONE_SCOPED

    Svt64Node* current = &nodes[0];

    for (uint32_t level = 1u; level <= depth; ++level) {
        const uint32_t x_index = (x >> ((depth - level) * 2u)) & 3u;
        const uint32_t y_index = (y >> ((depth - level) * 2u)) & 3u;
        const uint32_t z_index = (z >> ((depth - level) * 2u)) & 3u;

        const uint32_t child_index = (x_index << 0u) | (z_index << 2u) | (y_index << 4u);
        if ((current->child_mask & (1ull << child_index)) == 0u) {
            return nullptr;
        } else if (level == depth) {
            const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
            return &physics_data[current->abs_ptr() + child_pos];
        }

        const uint32_t child_pos = (uint32_t)__popcnt64(current->child_mask & ((1ull << child_index) - 1u));
        current = &nodes[current->abs_ptr() + child_pos];
    }

    return nullptr;
}

void Svt64::build(const RawVoxels& raw_data) {
    TMT_ZONE_SCOPED

    /* Delete old data */
    if (depth > 0u) {
        delete[] nodes;
        delete[] materials;
        delete[] physics_data;
        depth = 0u;
    }

    /* Calculate the parameters of the 64 tree */
    depth = tree_depth(raw_data.w, raw_data.h, raw_data.d);
    const uint32_t max_nodes = max_node_count(depth);
    const uint32_t raw_voxels = raw_voxel_count(raw_data);

    /* Allocate space for new tree */
    nodes = new Svt64Node[max_nodes];
    node_count = 1u;
    materials = new MaterialIndex[raw_voxels + SVT64_BUFFER_MEMORY];
    physics_data = new PhysicsVoxel[raw_voxels + SVT64_BUFFER_MEMORY];
    voxels_capacity = raw_voxels + SVT64_BUFFER_MEMORY;
    voxel_count = 0u;

    /* Copy the material palette */
    palette = raw_data.palette;

    /* Begin the recursive build */
    nodes[0] = subdivide(raw_data, depth * 2u, glm::uvec3(0u));

    /* Reallocate the nodes to save memory */
    nodes = (Svt64Node*)realloc(nodes, (node_count + SVT64_BUFFER_MEMORY) * sizeof(Svt64Node));
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
inline uint32_t popcnt_var64(uint64_t mask, uint32_t width) { return (uint32_t)__popcnt64(mask & ((1ull << width) - 1)); }

/* Get the index of the voxel at a given position and traversal scale. */
glm::uvec3 voxel_index(glm::vec3 pos, uint32_t scale_exp) {
    /* Create a bitmask of all the position bits */
    const uint32_t pos_mask = (1u << (23 - scale_exp)) - 1u;
    const uint32_t cell_x = (uint32_t&)pos.x >> scale_exp & pos_mask;
    const uint32_t cell_y = (uint32_t&)pos.y >> scale_exp & pos_mask;
    const uint32_t cell_z = (uint32_t&)pos.z >> scale_exp & pos_mask;
    return glm::uvec3(cell_x, cell_y, cell_z);
}

Svt64Hit Svt64::trace(const Ray& ray) const {
    /* Traversal state */
    uint32_t stack[11] {};
    int scale_exp = 21;       /* 23 mantissa bits - 2 */
    uint32_t node_index = 0u; /* root node */
    Svt64Node node = nodes[node_index];

    const glm::vec3 origin = mirror_pos(ray.origin, ray.dir);
    const glm::vec3 dir = ray.dir;

    /* Mirror coordinates to simplify cell intersections */
    uint32_t mirror_mask = 0x00;
    if (dir.x > 0.0f) mirror_mask |= 3u << 0;
    if (dir.y > 0.0f) mirror_mask |= 3u << 4;
    if (dir.z > 0.0f) mirror_mask |= 3u << 2;

    /* Safety clamp */
    glm::vec3 pos = clamp(origin, 1.0f, 1.9999999f);
    const glm::vec3 inv_dir = 1.0f / -glm::abs(dir);

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
        float tmax = fminf(fminf(side_dist.x, side_dist.y), side_dist.z);

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

        const float tmax = glm::min(glm::min(side_dist.x, side_dist.y), side_dist.z);
        const glm::bvec3 side_mask = glm::greaterThanEqual(glm::vec3(tmax), side_dist);
        const glm::vec3 normal = glm::vec3(side_mask.x ? -glm::sign(dir.x) : 0.0f, side_mask.y ? -glm::sign(dir.y) : 0.0f, side_mask.z ? -glm::sign(dir.z) : 0.0f);

        return Svt64Hit(pos, 0xFFFFFFFFu, normal, voxel_index(pos, scale_exp));
    }
    return Svt64Hit();
}

Svt64::~Svt64() {
    if (depth > 0u) {
        delete[] nodes;
        delete[] materials;
        delete[] physics_data;
        depth = 0u;
    }
}

/* Copy */
Svt64::Svt64(const Svt64& src) {
    node_count = src.node_count;
    voxel_count = src.voxel_count;
    nodes_wasted = src.nodes_wasted;
    voxels_wasted = src.voxels_wasted;
    nodes_capacity = src.nodes_capacity;
    voxels_capacity = src.voxels_capacity;
    depth = src.depth;
    nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY];
    materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY];
    physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY];
    memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
    memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
    memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
    palette = src.palette;
}

/* Copy */
Svt64& Svt64::operator=(const Svt64& src) {
    node_count = src.node_count;
    voxel_count = src.voxel_count;
    nodes_wasted = src.nodes_wasted;
    voxels_wasted = src.voxels_wasted;
    nodes_capacity = src.nodes_capacity;
    voxels_capacity = src.voxels_capacity;
    depth = src.depth;
    nodes = new Svt64Node[node_count + SVT64_BUFFER_MEMORY];
    materials = new MaterialIndex[voxel_count + SVT64_BUFFER_MEMORY];
    physics_data = new PhysicsVoxel[voxel_count + SVT64_BUFFER_MEMORY];
    memcpy(nodes, src.nodes, node_count * sizeof(Svt64Node));
    memcpy(materials, src.materials, voxel_count * sizeof(MaterialIndex));
    memcpy(physics_data, src.physics_data, voxel_count * sizeof(PhysicsVoxel));
    palette = src.palette;
    return *this;
}

void Svt64::set_voxel(const uint32_t x, const uint32_t y, const uint32_t z, const MaterialIndex material) {
    /* Check if we ran out of space */
    if (node_count >= nodes_capacity || voxel_count >= voxels_capacity) {
        static bool done_once = false;
        if (done_once == false) {
            Log::warn(Log::Scope::ENGINE, "SVT64 ran out of buffer memory.");
            done_once = true;
        }
        return;
    }

    /* Check if the voxel is in bounds */
    const uint32_t width = (uint32_t)powf(4.0f, (float)depth);
    if (x >= width || y >= width || z >= width) { 
        Log::warn("SVT64 tried to set voxel outside bounds.");
        return;
    }

	uint32_t node_index = 0u;
	uint32_t scale = depth * 2u - 2u;

	for (;;) {
		Svt64Node& node = nodes[node_index];

		if (node.is_leaf()) break;

		const uint32_t lx = (x >> scale) & 3u;
		const uint32_t ly = (y >> scale) & 3u;
		const uint32_t lz = (z >> scale) & 3u;
		const uint32_t li = lx + ly * 16u + lz * 4u;
		const uint64_t lm = 1ull << li;

		/* If the child doesn't exist yet, create it */
		if ((node.child_mask & lm) == 0ull) {
			const uint32_t prev_child_index = node.abs_ptr();
            const uint32_t prev_nodes_count = popcnt_var64(node.child_mask, 64u);
            if (node_count + prev_nodes_count + 64u >= nodes_capacity) return;
			nodes_wasted += prev_nodes_count;
			node = Svt64Node(false, node_count, node.child_mask | lm);

			for (int i = 0, j = 0; i < 64; ++i) {
				const bool is_new = i == li;

				if (is_new) {
					/* Add new node */
					nodes[node_count++] = Svt64Node(scale <= 2u, 0u, 0ull);
				} else {
					/* Copy over old node */
					if (node.child_mask & (1ull << i)) {
						nodes[node_count++] = nodes[prev_child_index + j];
						j++;
					}
				}
			}
		}

		scale -= 2u;
		node_index = node.abs_ptr() + popcnt_var64(node.child_mask, li);
	}

	Svt64Node& node = nodes[node_index];

	const uint32_t lx = x & 3u;
	const uint32_t ly = y & 3u;
	const uint32_t lz = z & 3u;
	const uint32_t li = lx + ly * 16u + lz * 4u;
	const uint64_t lm = 1ull << li;

	/* If the voxel doesn't exist yet, create it */
	if ((node.child_mask & lm) == 0ull) {
		const uint32_t prev_voxel_index = node.abs_ptr();
        const uint32_t prev_voxel_count = popcnt_var64(node.child_mask, 64u);
        if (voxel_count + prev_voxel_count + 64u >= voxels_capacity) return;
		voxels_wasted += prev_voxel_count;

		node = Svt64Node(true, voxel_count, node.child_mask | lm);
		for (int i = 0, j = 0; i < 64; ++i) {
			const bool is_new = i == li;

			if (is_new) {
				/* Add new voxel */
				materials[voxel_count++] = material;
			} else {
				/* Copy over old voxel */
				if (node.child_mask & (1ull << i)) {
					materials[voxel_count++] = materials[prev_voxel_index + j];
					j++;
				}
			}
		}
	} else {
        const uint32_t voxel_index = node.abs_ptr() + popcnt_var64(node.child_mask, li);
        materials[voxel_index] = material;
    }
}

void Svt64::defrag() {
	/* Copy current nodes and voxels */
	Svt64Node* new_nodes = new Svt64Node[node_count];
	MaterialIndex* new_materials = new MaterialIndex[voxel_count];
	PhysicsVoxel* new_physics_data = new PhysicsVoxel[voxel_count];

	/* Traverse the tree top down, move all child nodes/voxels into new lists */
	node_count = 1u;
	voxel_count = voxels_wasted = nodes_wasted = 0u;
	
    /* Non-recursive */
	uint32_t old_stack[128] {};
	uint32_t new_stack[128] {};
	uint32_t stack_ptr = 0u;
	uint32_t old_node_index = 0u;
	uint32_t new_node_index = 0u;

	for (;;) {
		const Svt64Node& old_node = nodes[old_node_index];
		Svt64Node& new_node = new_nodes[new_node_index];
		const uint32_t child_count = popcnt_var64(old_node.child_mask, 64u);
		const uint32_t child_ptr = old_node.abs_ptr();

		/* Defrag leaf node */
		if (old_node.is_leaf()) {
			new_node = Svt64Node(true, voxel_count, old_node.child_mask);
			memcpy(new_materials + voxel_count, materials + child_ptr, child_count * sizeof(MaterialIndex));
			voxel_count += child_count;
			if (stack_ptr == 0u) break;
			old_node_index = old_stack[--stack_ptr];
			new_node_index = new_stack[stack_ptr];
			continue;
		}

		/* Defrag node */
		new_node = Svt64Node(false, node_count, old_node.child_mask);
		memcpy(new_nodes + node_count, nodes + child_ptr, child_count * sizeof(Svt64Node));
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

	delete[] nodes;
	nodes = new_nodes;
	delete[] materials;
	materials = new_materials;
	delete[] physics_data;
	physics_data = new_physics_data;
}

}  // namespace tmt
