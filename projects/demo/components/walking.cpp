#include "walking.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

void Walking::start() { tmt::GoapAgentFactory::spawn_agent_from_type("dragon", entity); }

void Walking::update(const tmt::FrameData& time) {
    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& asteroid_pos = tmt::engine.ecs.get_component<tmt::Transform>(walkable_asteroid);
    glm::vec3 dir_to_asteroid = walking_transform.get_world_position() - asteroid_pos.get_world_position();
    tmt::Ray ray;
    ray.origin = walking_transform.get_world_position() + glm::normalize(dir_to_asteroid) * 2.0f;
    ray.dir = glm::normalize(-dir_to_asteroid);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray);
    tmt::engine.polyline.use_line_width(20.0f);
    tmt::engine.polyline.draw_arrow(ray.origin, ray.dir, hit.distance);
    if (hit.miss()) return;
    glm::vec3 move_to = ray.origin + ray.dir * hit.distance;
    glm::vec3 desired_velocity = glm::normalize(move_to - walking_transform.get_world_position()) * walk_speed * 0.5f;
    velocity += desired_velocity;

    velocity *= 0.9f;
    walking_transform.set_world_position(walking_transform.get_world_position() + velocity * time.delta_time);
    

    tmt::engine.polyline.draw_sphere(ray.origin + ray.dir * hit.distance, .2f);
}

void Walking::end() {}
