#include "walking.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"

void Walking::start() { tmt::GoapAgentFactory::spawn_agent_from_type("dragon", entity); }

void Walking::update(const tmt::FrameData& time) {
    tmt::Transform& walking_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& asteroid_pos = tmt::engine.ecs.get_component<tmt::Transform>(walkable_asteroid);
    glm::vec3 dir_to_asteroid = walking_transform.get_world_position() - asteroid_pos.get_world_position();

    // find closest point to asteroid
    float closest_distance = 1e34f;
    tmt::Hit closest_hit;
    tmt::Ray closest_ray;

    const float golden_ratio = (1.0f + std::sqrt(5.0f)) * 0.5f;
    const float angle_increment = 2.0f * glm::pi<float>() / golden_ratio;
    for (int i = 0; i < ray_count; ++i) {
        float t = (float)i / (float)ray_count;
        float y = 1.0f - 2.0f * t;
        float radius = std::sqrt(1.0f - y * y);
        float theta = angle_increment * i;

        float x = std::cos(theta) * radius;
        float z = std::sin(theta) * radius;

        glm::vec3 dir = glm::normalize(glm::vec3(x, y, z));
        tmt::Ray ray;
        ray.origin = walking_transform.get_world_position() + glm::normalize(dir_to_asteroid) * 0.5f;
        ray.dir = dir;

        const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray);
        if (hit.miss()) continue;

        if (closest_distance > hit.distance) {
            closest_distance = hit.distance;
            closest_hit = hit;
            closest_ray = ray;
        }
    }

    // height correction
    tmt::Ray ray;
    ray.origin = walking_transform.get_world_position() + glm::normalize(dir_to_asteroid) * 2.0f;
    ray.dir = glm::normalize(-dir_to_asteroid);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(ray);
    if (hit.miss()) return;

    glm::vec3 move_to = closest_ray.origin + closest_ray.dir * closest_hit.distance;
    glm::vec3 desired_velocity = glm::normalize(move_to - walking_transform.get_world_position()) * walk_speed * 0.5f;
    velocity += desired_velocity;

    // movement
    velocity *= 0.9f;
    walking_transform.set_world_position(walking_transform.get_world_position() + velocity * time.delta_time);

    // rotation    
    // use velocity to face the direction we are moving
    glm::quat rot_velocity;
    {
        glm::vec3 dir = glm::normalize(velocity);
        glm::vec3 forward = glm::vec3(0, 0, 1);
        float dot = glm::dot(forward, dir);

        if (dot > 0.9999f) {
            rot_velocity = glm::quat(1, 0, 0, 0);
        } else if (dot < -0.9999f) {
            rot_velocity = glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0));
        } else {
            glm::vec3 axis = glm::normalize(glm::cross(forward, dir));
            float angle = std::acos(dot);
            rot_velocity = glm::angleAxis(angle, axis);
        }
        rot_velocity = glm::normalize(rot_velocity);
    }
    
    // use ground to stay upright
    glm::quat rot_ground;
    {
        glm::vec3 ground_up = -glm::normalize(closest_ray.dir);
        glm::vec3 up = glm::vec3(0, 1, 0);
        float dot = glm::clamp(glm::dot(up, ground_up), -1.0f, 1.0f);

        if (dot > 0.9999f) {
            rot_ground = glm::quat(1, 0, 0, 0);
        } else if (dot < -0.9999f) {
            rot_ground = glm::angleAxis(glm::pi<float>(), glm::vec3(1, 0, 0));
        } else {
            glm::vec3 axis = glm::normalize(glm::cross(up, ground_up));
            float angle = std::acos(dot);
            rot_ground = glm::angleAxis(angle, axis);
        }
        rot_ground = glm::normalize(rot_ground);
    }

    // combine rotations
    glm::quat rot = glm::slerp(rot_ground, rot_velocity, rotation_t);

    float t = time.delta_time * rotation_speed;
    rotation = glm::slerp(rotation, rot, t);
    rotation = glm::normalize(rotation);
    walking_transform.set_world_rotation(rotation);

    glm::vec3 half_extent = {0.2f, 0.2f, 0.5f};

    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(2.0f);
    tmt::engine.polyline.draw_obb(walking_transform.get_world_position(), half_extent, walking_transform.get_world_rotation());
}

void Walking::end() {}
