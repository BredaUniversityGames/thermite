#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {


class Volume {
   public:
    glm::uvec3 size;
    std::vector<bool> occupied;
    uint32_t divisor;

    void traverse(tmt::ResourceRef<tmt::VoxelVolume> voxel_volume, int lod_level) {
        uint32_t full_depth = voxel_volume->blas->depth;
        uint32_t target_depth = (lod_level < (int)full_depth) ? (full_depth - lod_level) : 1;

        divisor = 1u << (lod_level * 2);
        size = (voxel_volume->size + divisor - 1u) / divisor;

        occupied.assign(size.x * size.y * size.z, false);
        auto set_occupied = [&](glm::uvec3 pos) {
            if (pos.x < size.x && pos.y < size.y && pos.z < size.z) {
                occupied[pos.x + size.x * pos.y + size.x * size.y * pos.z] = true;
            }
        };

        std::function<void(uint32_t, glm::uvec3, uint32_t, uint32_t)> traverse = [&](uint32_t node_idx, glm::uvec3 origin, uint32_t scale, uint32_t depth) {
            const auto& node = voxel_volume->blas->nodes[node_idx];
            uint64_t mask = node.child_mask;
            uint32_t base = node.abs_ptr();

            for (int i = 0; mask; ++i, mask >>= 1) {
                if (!(mask & 1)) continue;

                glm::uvec3 local = {i & 3, (i >> 4) & 3, (i >> 2) & 3};
                glm::uvec3 child_origin = origin + local * scale;

                // Stop early at target depth, or if we hit a leaf
                if (depth >= target_depth || node.is_leaf()) {
                    set_occupied(child_origin / divisor);
                } else {
                    uint32_t child_idx = base + std::popcount(node.child_mask & ((1ull << i) - 1));
                    traverse(child_idx, child_origin, scale / 4, depth + 1);
                }
            }
        };
        uint32_t scale = 1u << ((full_depth - 1) * 2);
        traverse(0, {0, 0, 0}, scale, 1);
    }

    bool voxel_empty(uint32_t x, uint32_t y, uint32_t z) { return !occupied[x + size.x * y + size.x * size.y * z]; };
    bool is_surface(uint32_t x, uint32_t y, uint32_t z) {
        if (voxel_empty(x, y, z)) return false;
        for (size_t sign = 0; sign < 2; sign++) {
            for (size_t dir = 0; dir < 3; dir++) {
                glm::uvec3 temp = {x, y, z};
                temp[(int)dir] += sign == 0 ? (int)1 : -((int)1);
                if (!(temp.x < size.x && temp.y < size.y && temp.z < size.z) || voxel_empty(temp.x, temp.y, temp.z)) {
                    return true;
                }
            }
        }
        return false;
    };
};


enum class VoxelType : uint8_t { EMPTY, OUTSIDE, INSIDE };

struct NavVoxel {
    VoxelType type = VoxelType::EMPTY;
};

class Node {
   public:
    glm::vec3 local_pos;
    glm::vec3 world_pos;
    int parent = 0;

    uint32_t voxel_index = 0;

    float g = -1.0f;
    float h = 0.0f;
    int id = -1;

    std::vector<int> connecting_nodes;
};

class NavMesh {
   public:
    NavMesh() = default;
    ~NavMesh() = default;

    std::vector<Node> nodes;
    std::vector<int> path;

    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume;
    Volume* volume;
    int lod_level = 0;

    bool generating = false;
   private:
    int iteration_nmg = 0;
    bool generation_complete = false;
    bool entered_loop = false;
   public:

    void generate_mesh_over_time();
    void generate_mesh(int iterations = 1e34);
    std::vector<int> find_path(const int starting_node_id, const int ending_node_id);
    int find_closest_node(const glm::vec3& position);
    std::optional<glm::vec3> follow_path(glm::vec3 start, glm::vec3 end);
    void inspect();
};
}  // namespace tmt

TMT_COMPONENT(tmt::NavMesh, "Navigation Mesh", (voxel_volume, lod_level));