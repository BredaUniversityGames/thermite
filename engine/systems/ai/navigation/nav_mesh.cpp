#include "nav_mesh.hpp"

#include "engine/core/resources/voxel_volume.hpp"
#include "engine/engine.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/renderer/pipelines/debug_pipeline.hpp"
#include "engine/core/renderer/renderer.hpp"

using namespace tmt;

void NavMesh::generate_mesh(IO::FileLocation location) {
    auto voxel_file = engine.resources.load_resource<VoxelScene>(location);
    auto voxel_volume = engine.resources.copy_resource<VoxelVolume>(voxel_file);

    { /* Voxel entity */
        auto voxel = engine.ecs.create_entity("Moving Voxel");
        auto& transform = engine.ecs.add_component<Transform>(voxel);
        auto& renderer = engine.ecs.add_component<VoxelRenderer>(voxel);
        renderer.resource = voxel_volume;
        transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
        transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    }

    // Set size FIRST
    auto size = voxel_volume->size;

    // Build dense occupancy grid from sparse tree
    std::vector<bool> occupied(size.x * size.y * size.z, false);

    auto set_occupied = [&](glm::uvec3 pos, MaterialIndex) {
        if (pos.x < size.x && pos.y < size.y && pos.z < size.z) occupied[pos.x + size.x * pos.y + size.x * size.y * pos.z] = true;
    };

    // Traverse the SVT
    std::function<void(uint32_t, glm::uvec3, uint32_t)> traverse = [&](uint32_t node_idx, glm::uvec3 origin, uint32_t scale) {
        const auto& node = voxel_volume->blas->nodes[node_idx];
        uint64_t mask = node.child_mask;
        uint32_t base = node.abs_ptr();

        for (int i = 0; mask; ++i, mask >>= 1) {
            if (!(mask & 1)) continue;

            glm::uvec3 local = {i & 3, (i >> 2) & 3, (i >> 4) & 3};
            glm::uvec3 child_origin = origin + local * scale;
            uint32_t child_idx = base + std::popcount(node.child_mask & ((1ull << i) - 1));

            if (node.is_leaf()) {
                set_occupied(child_origin, voxel_volume->blas->voxels[child_idx]);
            } else {
                traverse(child_idx, child_origin, scale / 4);
            }
        }
    };

    uint32_t scale = 1u << ((voxel_volume->blas->depth - 1) * 2);
    traverse(0, {0, 0, 0}, scale);

    // Helper lambda
    auto voxel_empty = [&](uint32_t x, uint32_t y, uint32_t z) { return !occupied[x + size.x * y + size.x * size.y * z]; };

    // Initialize data vector
    auto data = std::vector<NavVoxel>(size.x * size.y * size.z);

    std::unordered_map<uint32_t, int> node_map;

    for (uint32_t z = 0; z < size.z; z++) {
        for (uint32_t y = 0; y < size.y; y++) {
            for (uint32_t x = 0; x < size.x; x++) {
                const uint32_t pos_index = x + size.x * y + size.x * size.y * z;
                if (voxel_empty(x, y, z)) continue;

                int node_index = -1;
                if (node_map.contains(pos_index)) {
                    node_index = node_map[pos_index];
                } else {
                    // Create node
                    Node node;
                    node_map[pos_index] = (int)nodes.size();
                    node_index = (int)nodes.size();
                    nodes.push_back(node);
                }

                glm::ivec3 normal = {};
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
                                node_map[connecting_pos_index] = (int)nodes.size();
                                connecting_node_index = (int)nodes.size();
                                nodes.push_back(node);
                            }

                            nodes[node_index].connectingNodes.push_back(connecting_node_index);
                        }
                    }
                }
            }
        }
    }
}

std::vector<glm::vec3> NavMesh::find_path(const int startingNodeID, const int endingNodeID) {
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
            float edgeCost = glm::distance(nodes[next].pos, nodes[current].pos);

            // if g is not set or we found a better path to next node
            if (nodes[next].g < 0 || nodes[current].g + edgeCost < nodes[next].g) {
                nodes[next].g = nodes[current].g + edgeCost;
                nodes[next].h = glm::distance(nodes[next].pos, nodes[endingNodeID].pos);
                nodes[next].parent = current;

                open.push_back(next);
            }
        }
    }

    // no path found
    if (current != endingNodeID) {
        std::vector<glm::vec3> noResult;
        return noResult;
    }

    // reconstruct path and return it
    std::vector<glm::vec3> path;
    path.push_back(nodes[endingNodeID].pos);
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
        path.push_back(nodes[nodes[current].parent].pos);
        current = nodes[current].parent;
    }
}

void tmt::NavMesh::inspect() {
    for (int i = 0; i < nodes.size(); i++) {
        for (int j = 0; j < nodes[i].connectingNodes.size(); j++) {
            int conn_idx = nodes[i].connectingNodes[j];
            glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
            if (nodes[i].too_steep || nodes[j].too_steep) color = glm::vec3(1.0f, 0.0f, 0.0f);
            engine.renderer.draw_line(nodes[i].pos, nodes[conn_idx].pos, color);
        }
    }
}