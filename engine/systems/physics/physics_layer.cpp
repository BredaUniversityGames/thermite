#include "physics_layer.hpp"

namespace tmt {

PhysicsLayers::PhysicsLayers() {
    // Initialize collision matrix to false
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        for (uint32_t j = 0; j < MAX_LAYERS; ++j) {
            collision_matrix[i][j] = false;
        }
    }
}

void PhysicsLayers::enable_collision(uint32_t a, uint32_t b, bool enable) {
    if (a >= MAX_LAYERS || b >= MAX_LAYERS) return;

    collision_matrix[a][b] = enable;
    collision_matrix[b][a] = enable;  // enforce symmetry
}

bool PhysicsLayers::can_collide(uint32_t a, uint32_t b) const {
    if (a >= MAX_LAYERS || b >= MAX_LAYERS) return false;

    return collision_matrix[a][b];
}

uint32_t PhysicsLayers::add_layer(const std::string& name) {
    if (layer_count >= MAX_LAYERS) throw std::runtime_error("Max physics layers reached");

    uint32_t new_layer = layer_count;
    layer_names[new_layer] = name;

    // By default, new layer collides with all existing layers
    for (uint32_t i = 0; i <= new_layer; ++i) {
        collision_matrix[new_layer][i] = true;
        collision_matrix[i][new_layer] = true;
    }

    layer_count++;
    return new_layer;
}

void PhysicsLayers::remove_layer(uint32_t layer) {
    if (layer >= layer_count) return;

    // Disable all collisions involving this layer
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        collision_matrix[layer][i] = false;
        collision_matrix[i][layer] = false;
    }

    layer_names[layer].clear();
}

const std::string& PhysicsLayers::get_layer_name(uint32_t layer) const {
    if (layer >= MAX_LAYERS) throw std::out_of_range("Invalid physics layer");

    return layer_names[layer];
}

}  // namespace tmt
