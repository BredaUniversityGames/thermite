#include "asteroid_field_component.hpp"
#include "engine/tools/random.hpp"
#include "engine/tools/prefab_helper.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

std::vector<AsteroidFieldComponent::AsteroidPoissonPoint> AsteroidFieldComponent::get_poisson_points(const FAsteroidFieldLayer& layer) {
    std::vector<AsteroidPoissonPoint> radii_points;

    if (layer.layer_entries.empty()) return radii_points;

    float cell_size = layer.spacing_radius / sqrtf(3.f);

    glm::vec3 region_size = glm::vec3(layer.end_r * 2.f, layer.end_r * 2.f, layer.end_r * 2.f);

    SpatialLookupGrid grid(cell_size, region_size);

    glm::vec3 inside_annulus = get_belt_biased_unit_vector(layer.belt_bias) * (layer.start_r + ((layer.end_r - layer.start_r) * 0.5f));

    float max_size_factor = 0.f;
    for (auto& layer : layer.layer_entries) {
        if (layer.radius_factor > max_size_factor) max_size_factor = layer.radius_factor;
    }
    float max_radius = layer.spacing_radius * max_size_factor;

    int it = 1;
    while (it > 0) {
        bool found_candidate = false;
        for (int i = 0; i < layer.rejection_samples; i++) {

            int entry_idx = Random::rand_range(0, layer.layer_entries.size() - 1);

            glm::vec3 candidate_point = get_belt_biased_unit_vector(layer.belt_bias) * Random::rand_range(layer.start_r + layer.layer_spacing, layer.end_r - layer.layer_spacing);

            float candidate_radius = layer.layer_entries[entry_idx].radius_factor * layer.spacing_radius;

            if (point_valid(candidate_point, candidate_radius, max_radius, grid, layer, radii_points)) {
                AsteroidPoissonPoint a_p;
                a_p.entry_idx = entry_idx;
                a_p.pos = candidate_point;
                a_p.radius = candidate_radius;

                radii_points.push_back(a_p);
                it++;

                candidate_point += region_size * 0.5f;

                int cell_x = static_cast<int>(candidate_point.x / cell_size);
                int cell_y = static_cast<int>(candidate_point.y / cell_size);
                int cell_z = static_cast<int>(candidate_point.z / cell_size);

                int idx = cell_x * grid.z_dim * grid.x_dim + cell_y * grid.x_dim + cell_z;
                grid[idx] = radii_points.size();

                found_candidate = true;
                break;
            }
        }

        if (!found_candidate) {
            it--;
        }
    }

    return radii_points;
}

glm::vec3 AsteroidFieldComponent::get_belt_biased_unit_vector(float bias) {
    glm::vec3 unit_vec = Random::rand_unit_vec();
    glm::vec3 up_vec = glm::vec3(0.f, 1.f, 0.f);

    glm::vec3 xy_vec = glm::normalize(unit_vec - up_vec * (glm::dot(unit_vec, up_vec)));
   
    return glm::normalize(glm::mix(unit_vec, xy_vec, bias));
}

bool AsteroidFieldComponent::point_valid(
   glm::vec3 candidate, float candidate_radius, float max_radius, const SpatialLookupGrid& grid, const FAsteroidFieldLayer& layer, const std::vector<AsteroidPoissonPoint>& points
) {
    candidate += grid.region_size * 0.5f;

    int cell_x = static_cast<int>(candidate.x / grid.cell_size);
    int cell_y = static_cast<int>(candidate.y / grid.cell_size);
    int cell_z = static_cast<int>(candidate.z / grid.cell_size);

    int search_dim =  std::ceil(max_radius / grid.cell_size) + 1;

    int start_search_x = std::max(cell_x - search_dim, 0);
    int end_search_x = std::min(cell_x + search_dim + 1, grid.x_dim);
    int start_search_y = std::max(cell_y - search_dim, 0);
    int end_search_y = std::min(cell_y + search_dim + 1, grid.y_dim);
    int start_search_z = std::max(cell_z - search_dim, 0);
    int end_search_z = std::min(cell_z + search_dim + 1, grid.z_dim);

    for (int x = start_search_x; x < end_search_x; x++) {
        for (int y = start_search_y; y < end_search_y; y++) {
            for (int z = start_search_z; z < end_search_z; z++) {
                int idx = x * grid.z_dim * grid.x_dim + y * grid.x_dim + z;
                int existing_point_index = grid[idx] - 1;

                if (existing_point_index != -1) {
                    glm::vec3 temp = (points[existing_point_index].pos) - (candidate - grid.region_size* 0.5f);
                    float sqr_distance = glm::dot(temp, temp);

                    float min_allowed = candidate_radius + points[existing_point_index].radius;

                    if (sqr_distance < min_allowed * min_allowed) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void AsteroidFieldComponent::spawn_field() 
{
    if (asteroid_layers.empty()) return;

    float cum_weight = 0.f;
    for (int i = 0; i < asteroid_layers.size(); i++) {
        cum_weight += asteroid_layers[i].space_weight;
    }

    auto& zero_layer = asteroid_layers[0];
    zero_layer.start_r = 0.f;
    zero_layer.end_r = field_radius * (zero_layer.space_weight / cum_weight);

    for (int i = 1; i < asteroid_layers.size(); i++) {
        auto& layer = asteroid_layers[i];
        auto& prev_layer = asteroid_layers[i - 1];

        layer.start_r = prev_layer.end_r;
        layer.end_r = prev_layer.end_r + field_radius * (layer.space_weight / cum_weight);
    }

    // poisson distribution sampling
    for (auto& layer : asteroid_layers) {
        std::vector<AsteroidPoissonPoint> radii_points = get_poisson_points(layer);

        for (auto& point : radii_points) {

            auto spawn_obj = layer.layer_entries[point.entry_idx].spawnable;
            float size_factor = layer.layer_entries[point.entry_idx].radius_factor;

            float pitch = Random::rand_range(0.f, 360.f);
            float yaw = Random::rand_range(0.f, 360.f);
            float roll = Random::rand_range(0.f, 360.f);

            auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj->file_location, entity);

            auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);
            
            transform.set_local_position(point.pos);
            transform.set_local_rotation(glm::vec3(pitch, yaw, roll));
        }
    }
}

void AsteroidFieldComponent::clear_children() 
{
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    for(auto child : transform.get_all_children())
    {
        tmt::engine.ecs.destroy_entity(child);
    }
}

void AsteroidFieldComponent::start() {}

void AsteroidFieldComponent::update(const tmt::FrameData& time) {}

void AsteroidFieldComponent::end() {}

AsteroidFieldComponent::SpatialLookupGrid::SpatialLookupGrid(float _cell_size, glm::vec3 _region_size) 
{
    region_size = _region_size;
    cell_size = _cell_size;

    x_dim = std::ceil(region_size.x / cell_size);
    y_dim = std::ceil(region_size.y / cell_size);
    z_dim = std::ceil(region_size.z / cell_size);

    int num_grid = x_dim * y_dim * z_dim;
    grid = std::vector<int>(num_grid);
}
