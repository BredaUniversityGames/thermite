#include "nav_mesh.hpp"


#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/logger.hpp"

using namespace tmt;

void NavMesh::generate_mesh(std::shared_ptr<tmt::VoxelVolume> voxel_volume, glm::vec3 pivot, int lod_level) {
    Log::warn("Volume size:  {}, {}, {}", voxel_volume->size.x, voxel_volume->size.y, voxel_volume->size.z);
    Log::warn("Depth: {}", voxel_volume->blas->depth);
    Log::warn("Voxel count: {}", voxel_volume->blas->voxel_count);

    nodes.clear();

    uint32_t full_depth = voxel_volume->blas->depth;
    uint32_t target_depth = (lod_level < full_depth) ? (full_depth - lod_level) : 1;

    // Each level is 4x coarser, so divide size accordingly
    uint32_t divisor = 1u << (lod_level * 2);                         // 1, 4, 16, 64...
    glm::uvec3 size = (voxel_volume->size + divisor - 1u) / divisor;  // round up

    std::vector<bool> occupied(size.x * size.y * size.z, false);

    auto set_occupied = [&](glm::uvec3 pos) {
        if (pos.x < size.x && pos.y < size.y && pos.z < size.z) {
            occupied[pos.x + size.x * pos.y + size.x * size.y * pos.z] = true;
        } else {
            Log::warn(Log::Scope::ENGINE, "Rejected voxel at ({},{},{}) - size is ({},{},{})", pos.x, pos.y, pos.z, size.x, size.y, size.z);
        }
    };

    glm::vec3 pivot_grid = pivot / float(divisor);

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

    // Helper lambda
    auto voxel_empty = [&](uint32_t x, uint32_t y, uint32_t z) { return !occupied[x + size.x * y + size.x * size.y * z]; };


    Log::warn(Log::Scope::ENGINE, "Grid size: ({},{},{})", size.x, size.y, size.z);

    for (uint32_t z = 0; z < size.z; z++) {
        uint32_t count = 0;
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t x = 0; x < size.x; x++) {
                if (!voxel_empty(x, y, z)) count++;
            }
        }
        Log::warn(Log::Scope::ENGINE, "Layer z={}: {} occupied", z, count);
    }



    // Initialize data vector
    auto data = std::vector<NavVoxel>(size.x * size.y * size.z);
    std::unordered_map<uint32_t, int> node_map;

    constexpr float voxel_scale = 0.1f;               // 1/10 = 0.1 units per voxel
    float cell_scale = voxel_scale * float(divisor);  // size of one LOD cell in world units
    glm::vec3 grid_size = glm::vec3(size);  // LOD grid dimensions

    for (uint32_t z = 0; z < size.z; z++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t x = 0; x < size.x; x++) {
                const uint32_t pos_index = x + size.x * y + size.x * size.y * z;
                if (voxel_empty(x, y, z)) continue;

                auto voxel_empty = [&](uint32_t x, uint32_t y, uint32_t z) { return !occupied[x + size.x * y + size.x * size.y * z]; };

                int sides = 6;
                for (size_t sign = 0; sign < 2; sign++) {
                    for (size_t dir = 0; dir < 3; dir++) {
                        glm::uvec3 temp = {x, y, z};
                        temp[(int)dir] += sign == 0 ? (int)1 : -((int)1);
                        if (!(temp.x >= 0 && temp.x < size.x && temp.y >= 0 && temp.y < size.y && temp.z >= 0 && temp.z < size.z) || voxel_empty(temp.x, temp.y, temp.z)) {
                            sides--;
                        }
                    }
                }
                if (sides == 6) {
                    continue;
                }

                int node_index = -1;
                if (node_map.contains(pos_index)) {
                    node_index = node_map[pos_index];
                } else {
                    Node node;
                    glm::vec3 grid_pos = glm::vec3(x, y, z) + 0.5f;
                    glm::vec3 centered = grid_pos - grid_size * 0.5f;
                    node.local_pos = glm::vec3(centered.x * cell_scale, centered.y * cell_scale, centered.z * cell_scale);

                    //glm::vec3 grid_pos = glm::vec3(x, y, z) + 0.5f;
                    //glm::vec3 centered = grid_pos - pivot_grid;
                    //node.local_pos = glm::vec3(centered.x * cell_scale - size.x / 1, centered.z * cell_scale - size.z / 1, centered.y * cell_scale - size.y * 1);

                    node_map[pos_index] = (int)nodes.size();
                    node_index = (int)nodes.size();
                    nodes.push_back(node);
                }

                for (size_t sign = 0; sign < 2; sign++) {
                    for (size_t dir = 0; dir < 3; dir++) {
                        glm::uvec3 temp = {x, y, z};
                        temp[(int)dir] += sign == 0 ? (int)1 : -((int)1);
                        const uint32_t connecting_pos_index = temp.x + size.x * temp.y + size.x * size.y * temp.z;

                        if ((temp.x < size.x && temp.y < size.y && temp.z < size.z) && !voxel_empty(temp.x, temp.y, temp.z)) {
                            int connecting_node_index = -1;
                            if (node_map.contains(connecting_pos_index)) {
                                connecting_node_index = node_map[connecting_pos_index];
                            } else {
                                Node node;
                                glm::vec3 grid_pos = glm::vec3(x, y, z) + 0.5f;
                                glm::vec3 centered = grid_pos - grid_size * 0.5f;
                                node.local_pos = glm::vec3(centered.x * cell_scale, centered.y * cell_scale, centered.z * cell_scale);

                                //glm::vec3 grid_pos = glm::vec3(x, y, z) + 0.5f;
                                //glm::vec3 centered = grid_pos - pivot_grid;
                                //node.local_pos = glm::vec3(centered.x * cell_scale - size.x / 1, centered.z * cell_scale - size.z / 1, centered.y * cell_scale - size.y * 1);
                                node_map[connecting_pos_index] = (int)nodes.size();
                                connecting_node_index = (int)nodes.size();
                                nodes.push_back(node);
                            }

                            auto& neighborsA = nodes[node_index].connectingNodes;
                            if (std::find(neighborsA.begin(), neighborsA.end(), connecting_node_index) == neighborsA.end()) {
                                neighborsA.push_back(connecting_node_index);
                            }

                            // Add node_index to connecting_node_index's list
                            auto& neighborsB = nodes[connecting_node_index].connectingNodes;
                            if (std::find(neighborsB.begin(), neighborsB.end(), node_index) == neighborsB.end()) {
                                neighborsB.push_back(node_index);
                            }
                        }
                    }
                }
            }
        }
    }
}

std::vector<int> NavMesh::find_path(const int startingNodeID, const int endingNodeID) {
    std::vector<int> open = std::vector<int>();
    open.push_back(startingNodeID);
    nodes[startingNodeID].g = 0.0f;
    nodes[startingNodeID].parent = NULL;
    int current = -1;

    // A* search
    // g is the cost from start to current node
    // h is the heuristic cost from current node to end node
    while (!open.empty()) {
        current = -1;
        int currentIndex = -1;

        // find node in open list with lowest f = g + h
        for (int i = 0; i < (int)open.size(); i++) {
            // set first node as a valid node
            if (current < 0) {
                currentIndex = i;
                current = open[i];
            }
            // get a better node if available
            else if (nodes[open[i]].g + nodes[open[i]].h < nodes[current].g + nodes[current].h) {
                currentIndex = i;
                current = open[i];
            }
        }

        if (current == endingNodeID) {
            break;
        }

        // remove current from open list
        open.erase(open.begin() + currentIndex);

        // add all connecting nodes with a better path to open list
        for (int i = 0; i < (int)nodes[current].connectingNodes.size(); i++) {
            int next = nodes[current].connectingNodes[i];
            float edgeCost = glm::distance(nodes[next].local_pos, nodes[current].local_pos);

            // if g is not set or we found a better path to next node
            if (nodes[next].g < 0 || nodes[current].g + edgeCost < nodes[next].g) {
                nodes[next].g = nodes[current].g + edgeCost;
                nodes[next].h = glm::distance(nodes[next].local_pos, nodes[endingNodeID].local_pos);
                nodes[next].parent = current;

                open.push_back(next);
            }
        }
    }

    // no path found
    if (current != endingNodeID) {
        std::vector<int> noResult;
        return noResult;
    }

    // reconstruct path and return it
    std::vector<int> path;
    path.push_back(endingNodeID);
    current = endingNodeID;
    while (1) {
        if (current == startingNodeID) {
            for (int i = 0; i < (int)nodes.size(); i++) {
                nodes[i].g = -1.0f;
                nodes[i].h = 0.0f;
                nodes[i].parent = 0;
            }
            return path;
        }
        path.push_back(nodes[current].parent);
        current = nodes[current].parent;
    }
}

void tmt::NavMesh::inspect() {
    for (int i = 0; i < nodes.size(); i++) {
        for (int j = 0; j < nodes[i].connectingNodes.size(); j++) {
            int conn_idx = nodes[i].connectingNodes[j];
            glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
            engine.renderer.draw_line(nodes[i].world_pos, nodes[conn_idx].world_pos, color);
        }
    }
}