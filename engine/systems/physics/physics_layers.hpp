#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>

namespace tmt {

constexpr uint32_t MAX_LAYERS = 32;

class PhysicsLayers {
   public:
    PhysicsLayers();

    void enable_collision(uint32_t a, uint32_t b, bool enable);
    bool can_collide(uint32_t a, uint32_t b) const;

    uint32_t add_layer(const std::string& name);
    void remove_layer(uint32_t layer);

    const std::string& get_layer_name(uint32_t layer) const;
    void set_layer_name(uint32_t layer, const std::string& name);

    uint32_t get_layer_count() const { return layer_count; }

    void load();
    void save() const;

   private:
    bool collision_matrix[MAX_LAYERS][MAX_LAYERS];
    std::string layer_names[MAX_LAYERS];
    uint32_t layer_count = 0;
};

}  // namespace tmt
