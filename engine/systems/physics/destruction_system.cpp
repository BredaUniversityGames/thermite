#include "destruction_system.hpp"
#include "destruction_system.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/physics/physics_system.hpp"

namespace tmt {

void Destruction::on_start() {
    for (const auto& [entity, des, vb] : engine.ecs.view<Destructable, VoxelBody>().each()) {
        generate_connection_graph(des, vb.resource->blas.get());
    }
}

void Destruction::on_update(const FrameData& time) {
    for (const auto& [entity, des, vb] : engine.ecs.view<Destructable, VoxelBody>().each()) {
        engine.polyline.use_depth_testing(false);
        engine.polyline.use_color(0.1f, 0.3f, 0.9f);
        for (DestructionNode& node : des.nodes) {
            const glm::vec3 half_scale = glm::vec3(vb.width, vb.height, vb.depth) * 0.5f;
            const glm::vec3 pos = (glm::vec3)node.global_position * UNITS_PER_VOXEL + vb.position + 0.05f - half_scale;

            engine.polyline.draw_circle(pos, 0.033f * (1 << node.level));

            //if (node.level == 0) {
            //    engine.polyline.draw_circle(pos, 0.033f);
            //} else {
            //    engine.polyline.draw_circle(pos, 0.033f * 4.0f);
            //}
        }
    }
}

void Destruction::on_fixed_update(const FrameData& time) {}

void Destruction::on_end() {
    Log::info("Destruction on_end");
}

void Destruction::destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    // Get voxel body component
    VoxelBody* vb = engine.ecs.try_get_component<VoxelBody>(entity);
    if (vb == nullptr) return;

    // Subtract voxels from BLAS
    auto* resource = vb->resource.resource.get();
    resource->blas.get()->subtract(stencil, offset);
    resource->set_dirty();

    // Find where the objects seperate and creates new entities for each part
    std::vector<Entity> seperated_entities = find_seperations(entity, stencil, offset);

    // Recalculate physics data for all seperate voxel bodies
    for (Entity seperate_entity : seperated_entities) {
        VoxelBody& seperate_vb = engine.ecs.get_component<VoxelBody>(seperate_entity);
        Physics::recalculate_physics_data(seperate_vb);
    }
}

std::vector<Entity> Destruction::find_seperations(Entity entity, const Stencil* stencil, glm::ivec3 offset) {
    VoxelBody& vb = engine.ecs.get_component<VoxelBody>(entity);
    auto* resource = vb.resource.resource.get();
    Svt64* tree = resource->blas.get();

    const glm::ivec3 dirs[] = { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };
    std::vector<glm::uvec3> edge_indices;

    // Loop over the stencil and find all voxels where destruction happened next to
    for (uint32_t local_z = 0; local_z < stencil->size.z; local_z++) {
        for (uint32_t local_y = 0; local_y < stencil->size.y; local_y++) {
            for (uint32_t local_x = 0; local_x < stencil->size.x; local_x++) {
                uint32_t i = local_x + local_y * stencil->size.x + local_z * stencil->size.x * stencil->size.y;

                // if this voxel is empty, skip it
                if (stencil->data[i] == 0) continue;
                glm::ivec3 tree_pos = glm::ivec3(local_x, local_y, local_z) + offset;

                // If the world position of this voxel is outside the bounds of the resource, skip it
                if (tree_pos.x < 0 || tree_pos.y < 0 || tree_pos.z < 0 || tree_pos.x > resource->size.x - 1 || tree_pos.y > resource->size.y - 1 || tree_pos.z > resource->size.z - 1) continue;

                const uint32_t edge_vals[] = { 0, stencil->size.x - 1, 0, stencil->size.y - 1, 0, stencil->size.z - 1 };
                const glm::ivec3 local_pos(local_x, local_y, local_z);

                for (int d = 0; d < 6; d++) {
                    // If we are on the edge of the stencil, we can directly mark the neighboring voxel
                    uint32_t local_axis_val = local_pos[d / 2];
                    if (local_axis_val == edge_vals[d]) {
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (neighbor_pos.x >= resource->size.x || neighbor_pos.y >= resource->size.y || neighbor_pos.z >= resource->size.z) continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    } else {  // We are not on the edge of the stencil so only mark the neighboring voxel if its empty int the tree
                        glm::ivec3 neighbor_pos = local_pos + dirs[d] + offset;

                        // If the world position is outside the bounds of the resource, skip it
                        if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                        if (neighbor_pos.x >= resource->size.x || neighbor_pos.y >= resource->size.y || neighbor_pos.z >= resource->size.z) continue;

                        // If this voxel is empty in the tree, skip it
                        if (tree->get_physics_voxel(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z) == nullptr) continue;

                        edge_indices.push_back((glm::uvec3)neighbor_pos);
                    }
                }
            }
        }
    }

    // Loop over all the edge indices and do a flood fill to find all connected voxels
    // Max size 1.07gb
    std::vector<uint8_t> flood_grid(resource->size.x * resource->size.y * resource->size.z, 0);

    uint8_t current_mark = 1;
    for (const glm::uvec3& edge_index : edge_indices) {
        uint32_t i = edge_index.x + edge_index.y * resource->size.x + edge_index.z * resource->size.x * resource->size.y;
        // If this voxel is already marked, skip it
        if (flood_grid[i] != 0) continue;

        // Do a flood fill from this voxel and mark all connected voxels with the same index
        std::vector<glm::uvec3> stack { edge_index };
        while (!stack.empty()) {
            // Get current voxel from stack and remove it
            glm::uvec3 current = stack.back();
            stack.pop_back();

            // Calculate index of this voxel in the flood grid
            uint32_t current_i = current.x + current.y * resource->size.x + current.z * resource->size.x * resource->size.y;

            // If this voxel is already marked, or it does not exist in the tree, skip it
            if (flood_grid[current_i] != 0 || tree->get_physics_voxel(current.x, current.y, current.z) == nullptr) continue;

            // Mark this voxel
            flood_grid[current_i] = current_mark;

            // Add neighbors to the stack
            for (int d = 0; d < 6; d++) {
                glm::ivec3 neighbor_pos = glm::ivec3(current) + dirs[d];
                // If the world position is outside the bounds of the resource, skip it
                if (neighbor_pos.x < 0 || neighbor_pos.y < 0 || neighbor_pos.z < 0) continue;
                if (neighbor_pos.x >= resource->size.x || neighbor_pos.y >= resource->size.y || neighbor_pos.z >= resource->size.z) continue;
                stack.push_back((glm::uvec3)neighbor_pos);
            }
        }
        current_mark++;
    }

    if (current_mark > 2) Log::info("Seperation Found!");

    return std::vector<Entity> { entity };

    // for (size_t i = 0; i < stencil_size; i++) {
    //     // if this voxel is empty, skip it
    //     if (stencil->data[i] == 0) continue;

    //    uint32_t local_x = i % stencil->size.x;
    //    uint32_t local_y = (i / stencil->size.x) % stencil->size.y;
    //    uint32_t local_z = i / (stencil->size.x * stencil->size.y);

    //
    //    edge_indices.push_back(i);
    //}
}

bool all_children_solid(Svt64* tree, const uint32_t node_index) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Copy mask for iteration
    uint64_t mask = node.child_mask;

    // Return false if we are not fully solid
    if (mask != 0xFFFFFFFFFFFFFFFF) return false;

    // if we are a leaf node, and we are fully solid return true
    if (node.is_leaf()) return true;

    // Recurse into all chidren
    for (size_t i = 0; i < 64; i++) {
        if(!all_children_solid(tree, node.abs_ptr() + i)) return false;
    }

    return true;
}

void recurse_generate_connection(
    Destructable& graph, Svt64* tree, const uint32_t node_index, const uint32_t current_depth, const uint32_t node_scale, const uint32_t global_x, const uint32_t global_y,
    const uint32_t global_z
) {
    // Get current node
    const tmt::Svt64Node& node = tree->nodes[node_index];

    // Calculate new scale for child nodes
    const uint32_t child_scale = node_scale >> 2u;

    // Copy mask for iteration
    uint64_t mask = node.child_mask;

    const uint8_t level = tree->depth - current_depth;
    
    // Alwasy skip root node, check if all children are solid
    if (current_depth != 0 && all_children_solid(tree, node_index)) {
        // Create node in connection graph for this leaf node
        DestructionNode connection_node;
        connection_node.level = level;
        connection_node.tree_node = node_index;
        connection_node.global_position = glm::uvec3(global_x + node_scale / 2, global_y + node_scale / 2, global_z + node_scale / 2);

        // Find neighbors on all edges
        
        // Make function that gets the highset level neighbors

        graph.nodes.push_back(connection_node);

        // We are a full leaf node, we can stop recursion here
        return;
    }

    while (mask != 0u) {
        // Find index of first set bit
        const uint32_t i = std::countr_zero(mask);

        // Get local coordinates
        const uint32_t local_x = (i >> 0u) & 3u;
        const uint32_t local_y = (i >> 4u) & 3u;
        const uint32_t local_z = (i >> 2u) & 3u;

        // Get child node index
        const uint32_t child_node_offset = (uint32_t)__popcnt64(node.child_mask & ((1ull << i) - 1u));
        const uint32_t child_node_index = node.abs_ptr() + child_node_offset;

        // Recursively go deeper until child is a leaf
        if (current_depth + 1 < tree->depth) {
            recurse_generate_connection(
                graph, tree, child_node_index, current_depth + 1, child_scale, global_x + local_x * child_scale, global_y + local_y * child_scale, global_z + local_z * child_scale
            );
        } else {
            // We are a voxel
            const uint32_t x = global_x + local_x * child_scale;
            const uint32_t y = global_y + local_y * child_scale;
            const uint32_t z = global_z + local_z * child_scale;

            // Create node in connection graph for this leaf node
            DestructionNode connection_node;
            connection_node.level = level - 1u;
            connection_node.tree_node = child_node_index;
            connection_node.global_position = glm::uvec3(x, y, z);

            graph.nodes.push_back(connection_node);
        }

        // Clear bit
        mask &= ~(1ull << i);
    }
}

void Destruction::generate_connection_graph(Destructable& graph, Svt64* tree) {
    // Scale of the root node in voxels
    const uint32_t node_scale = (1u << (tree->depth * 2u));
    recurse_generate_connection(graph, tree, 0, 0, node_scale, 0, 0, 0);
}

}  // namespace tmt