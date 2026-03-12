#include "mining_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/tools/fmt/glm.hpp"
#include "glm/detail/_noise.hpp"
#include "glm/detail/_noise.hpp"
#include "glm/detail/_noise.hpp"
#include "glm/detail/_noise.hpp"

#include <cmath>
#include <engine/systems/physics/physics_system.hpp>

using namespace game;

std::vector<glm::vec3> MiningComponent::compute_ray_origins(const tmt::Transform& transform) const {
    auto ray_origin = transform.get_world_position();

    std::vector<glm::vec3> origins;
    origins.reserve(ray_cylinder.ray_amount);
    glm::vec3 up = transform.get_up();
    glm::vec3 right = transform.get_right();
    constexpr float golden_angle = 2.399963229728f;
    // TODO copied code from debug draw, might be able to simplify it
    // TODO can also be copied in a buffer at the start and only add to the new origin if sin and cos prove to be too  intensive
    for (size_t i = 0; i < ray_cylinder.ray_amount; i++) {
        float elapsed_time_index = static_cast<float>(i) + glm::mod(tmt::engine.frame_data().elapsed_time * ray_cylinder.rotating_speed, 1.0f);
        const float a0 = golden_angle * elapsed_time_index;
        const float radius = std::sqrt(elapsed_time_index / static_cast<float>(ray_cylinder.ray_amount)) * ray_cylinder.base_radius;

        origins.emplace_back(ray_origin + radius * (right * glm::cos(a0) + up * glm::sin(a0)));
    }
    return origins;
}
void MiningComponent::assign_database() {
    auto view = tmt::engine.ecs.view<OreProperties>();
    for (auto& viewElement : view) {
        auto& ore_properties = tmt::engine.ecs.get_component<OreProperties>(viewElement.entity);
        ore_database = &ore_properties.ores;
    }
}
void MiningComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&MiningComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().connect<&MiningComponent::on_stop_mining>(this);

    // get a pointer to the database
    assign_database();
}

void MiningComponent::update(const tmt::FrameData& time) {
    has_drawn_debug = true;
}

tmt::Transform* MiningComponent::get_transform() const {
    tmt::Transform* transform;
    if (ray_cylinder.base_transform_entity == entt::null) {
        transform = &tmt::engine.ecs.get_component<tmt::Transform>(entity);
    } else {
        transform = &tmt::engine.ecs.get_component<tmt::Transform>(ray_cylinder.base_transform_entity);
    }
    return transform;
}
void MiningComponent::draw_debug_lines() const {
    cfg.set_values();
    auto& polyline = tmt::engine.polyline;

    tmt::Transform* transform = get_transform();
    auto ray_origin = transform->get_world_position();

    glm::vec3 forward = transform->get_forward();

    polyline.draw_world_circle(ray_origin, forward, ray_cylinder.base_radius);
    polyline.draw_world_circle(ray_origin + ray_cylinder.ray_distance * forward, forward, ray_cylinder.base_radius);
    if (has_drawn_debug == false) {
        polyline.draw_line(last_ray.origin, last_ray.origin + last_hit.distance * last_ray.dir, 2.0f);
        for (const glm::vec3& origin : previous_computed_origins) {
            polyline.draw_line(origin, origin + ray_cylinder.ray_distance * forward, 2.0f);
        }
    }
}

void MiningComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::on_weapon_fired>(this);
    tmt::engine.ecs.get_dispatcher().sink<ReleaseShootEvent>().disconnect<&MiningComponent::on_stop_mining>(this);
}

void MiningComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;
    mine(e.direction);
}
void MiningComponent::on_stop_mining(const ReleaseShootEvent& e) {
    // TODO this will trigger no matter what the entity is for now
    previous_hit.is_mining = false;
    tmt::Log::info("Stopped mining, reset previous hits");
}

void MiningComponent::handle_ore(const tmt::Hit& hit) {
    auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
    tmt::Material* material = resource->blas->get_voxel(hit.coord.x, hit.coord.y, hit.coord.z);

    // TODO I have no idea why this triggers
    if (material == nullptr) {
        tmt::Log::error("Somehow the voxel that was supposed to be destroyed at {}, returns a nullptr", hit.coord);
        return;
    }
    auto ore_type = material->type;

    // switched voxel, reset
    if (previous_hit.is_mining == false || hit.coord != previous_hit.voxel_coord) {
        previous_hit.mining_time = tmt::engine.frame_data().elapsed_time;
        previous_hit.voxel_coord = hit.coord;
        previous_hit.type = ore_type;
        previous_hit.is_mining = true;
    }

    switch (ore_type) {
        case tmt::Material::Type::NONE:
        case tmt::Material::Type::THERMITE:
        case tmt::Material::Type::COPPER:
        case tmt::Material::Type::TITANIUM:
            auto ore_toughness = ore_database->at(ore_type).toughness;
            if (ore_toughness < tmt::engine.frame_data().elapsed_time - previous_hit.mining_time) {
                resource->blas->remove_voxel(previous_hit.voxel_coord.x, previous_hit.voxel_coord.y, previous_hit.voxel_coord.z);
                resource->set_dirty();
                tmt::Log::info("Destroyed voxel");
            }
            break;
    }

    has_drawn_debug = false;
}
void MiningComponent::mine(const glm::vec3& dir) {
    previous_computed_origins = compute_ray_origins(*get_transform());
    for (auto& origin : previous_computed_origins) {
        const tmt::Ray ray_cast = tmt::Ray(origin, dir);
        const tmt::Hit hit = tmt::engine.ecs.systems.get<tmt::Physics>().raycast(ray_cast, ray_mask);
        if (hit.miss() == false && hit.distance < ray_cylinder.ray_distance) {
            handle_ore(hit);
        }
    }
}
