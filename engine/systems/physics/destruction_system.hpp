#pragma once
#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/physics/components/destructable.hpp"

namespace tmt {

class Stencil;

class Destruction : public ISystem {
   public:
    Destruction() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Destruction System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    void destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset = glm::ivec3(0));

    static uint32_t pos_to_node_id(uint32_t x, uint32_t y, uint32_t z) { return (x + (y << 10) + (z << 20)); };

   private:
    void fill_edge_indices(std::vector<glm::uvec3>& edge_indices, Entity entity, const Stencil* stencil, glm::ivec3 offset);
    std::vector<Entity> find_seperations(Entity entity, const std::vector<glm::uvec3>& edge_indices, const Stencil* stencil, glm::ivec3 offset);
    std::vector<Entity> find_seperations(Entity entity, const std::vector<glm::uvec3>& edge_indices, Destructable& graph);
    void regenerate_connection_graph(Destructable& graph, Svt64* tree, const Stencil* stencil, glm::ivec3 offset);
    void generate_connection_graph(Destructable& graph, Svt64* tree);

};

}  // namespace tmt