#include "physics_layers.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

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

void PhysicsLayers::set_layer_name(uint32_t layer, const std::string& name) {
    if (layer >= MAX_LAYERS) return;
    layer_names[layer] = name;
}

void PhysicsLayers::load() {
    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "physics_layers.json" });
    if (file_data.empty()) return;

    tmt::json j;
    try {
        j = tmt::json::parse(file_data);
    } catch (...) {
        return;
    }

    layer_count = j["layer_count"].get<uint32_t>();

    // Clear previous data
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        layer_names[i].clear();
        for (uint32_t k = 0; k < MAX_LAYERS; ++k) collision_matrix[i][k] = false;
    }

    // Load names
    auto& names = j["names"];
    for (uint32_t i = 0; i < layer_count; ++i) {
        if (i < names.size() && !names[i].is_null())
            layer_names[i] = names[i].get<std::string>();
        else
            layer_names[i] = "";
    }

    // Load collisions
    auto& collisions = j["collisions"];
    for (uint32_t i = 0; i < layer_count; ++i) {
        for (uint32_t k = 0; k < layer_count; ++k) {
            if (i < collisions.size() && k < collisions[i].size() && !collisions[i][k].is_null())
                collision_matrix[i][k] = collisions[i][k].get<bool>();
            else
                collision_matrix[i][k] = false;
        }
    }
}

void PhysicsLayers::save() const {
    nlohmann::json j;

    j["layer_count"] = layer_count;

    // Layer names
    for (uint32_t i = 0; i < layer_count; ++i) {
        j["names"][i] = layer_names[i];
    }

    // Collision matrix (upper triangle)
    for (uint32_t i = 0; i < layer_count; ++i) {
        for (uint32_t k = i; k < layer_count; ++k) {
            j["collisions"][i][k] = collision_matrix[i][k];
        }
    }

    IO::write_text_file({ IO::Location::PROJECT, "physics_layers.json" }, j.dump(4));
    Log::info("Saved {} physics layers", layer_count);
}

}  // namespace tmt
