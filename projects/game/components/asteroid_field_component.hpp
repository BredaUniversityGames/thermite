#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/voxel_volume.hpp"


struct FAsteroidLayerEntry {
    tmt::ResourceRef<tmt::Json> spawnable;

    float radius_factor = 1.f;
};
TMT_OBJECT(FAsteroidLayerEntry, (spawnable, radius_factor));

struct FAsteroidFieldLayer {
    std::vector<FAsteroidLayerEntry> layer_entries;

    int rejection_samples = 20;

    float spacing_radius = 50.f;

    float space_weight = 1.f;

    float belt_bias = 1.f;

    float layer_spacing = 0.f;

    float start_r;
    float end_r;
};
TMT_OBJECT(FAsteroidFieldLayer, (layer_entries, rejection_samples, spacing_radius, space_weight, belt_bias, layer_spacing));

struct AsteroidFieldComponent : public tmt::GameComponent<AsteroidFieldComponent> {

    using GameComponent::GameComponent;

    float field_radius;
    std::vector<FAsteroidFieldLayer> asteroid_layers;

    struct AsteroidPoissonPoint {
        glm::vec3 pos;
        int entry_idx;
        float radius;
    };


    void spawn_field();
    void clear_children();

    static std::string_view get_name() { return "Asteroid Field Component"; }

    private:

    std::vector<AsteroidPoissonPoint> get_poisson_points(const FAsteroidFieldLayer& layer);
    glm::vec3 get_belt_biased_unit_vector(float bias);

    struct SpatialLookupGrid {
        SpatialLookupGrid(float cell_size, glm::vec3 region_size);

        std::vector<int> grid;
        int x_dim;
        int y_dim;
        int z_dim;

        float cell_size;
        glm::vec3 region_size;

        int& operator[](int idx) { return grid[idx]; }
        int operator[](int idx) const { return grid[idx]; }
    };

    bool point_valid(glm::vec3 point, float radius, float max_radius, const SpatialLookupGrid& grid, const FAsteroidFieldLayer& layer, const std::vector<AsteroidPoissonPoint>& points);


    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
};
TMT_OBJECT(AsteroidFieldComponent, (field_radius, asteroid_layers));