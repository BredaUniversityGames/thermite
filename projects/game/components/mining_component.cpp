#include "mining_component.hpp"
#include "engine/shared/ray.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/tools/fmt/glm.hpp"

using namespace game;

void MiningComponent::start() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().connect<&MiningComponent::on_weapon_fired>(this);
}

void MiningComponent::update(const tmt::FrameData& time) {
    has_drawn_debug = true;
}

void MiningComponent::draw_debug_lines() const {
    if (has_drawn_debug) return;
    cfg.set_values();
    auto& polyline = tmt::engine.polyline;
    polyline.draw_line(last_ray.origin, last_ray.origin + last_hit.distance * last_ray.dir, 2.0f);
    // polyline.draw_world_circle()
}

void MiningComponent::end() {
    tmt::engine.ecs.get_dispatcher().sink<WeaponFiredEvent>().disconnect<&MiningComponent::on_weapon_fired>(this);
}

void MiningComponent::on_weapon_fired(const WeaponFiredEvent& e) {
    if (e.weapon_entity != entity) return;
    mine(e.origin, e.direction);
}

void MiningComponent::mine(glm::vec3 origin, glm::vec3 dir) {
    if (!stencil) {
        tmt::Log::error("Mining component has no valid stencil!");
        return;
    }

    const tmt::Ray ray_cast = tmt::Ray(origin, dir);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray_cast);
    last_ray = ray_cast;
    last_hit = hit;
    // TODO maybe add a generic resource for the time

    if (hit.miss() == false) {
        auto* resource = tmt::engine.ecs.get_component<tmt::VoxelRenderer>(hit.entity).resource.resource.get();
        // TODO this is a band-aid until we have proper ids for materials
        auto* material = resource->blas->get_voxel(hit.coord.x, hit.coord.y, hit.coord.z);
        tmt::Log::info("Material to be mined is {}", material->albedo.unpack());
        has_drawn_debug = false;
    }
}
