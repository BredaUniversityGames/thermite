#include "steering_system.hpp"

#include <glm/gtx/norm.hpp>

#include "engine.hpp"
#include "core/logger.hpp"

namespace tmt {

void SteeringSystem::on_start() {
    Log::info("Steering on_start");
}

void SteeringSystem::on_fixed_update(const FrameData& time) {
    auto view = engine.ecs.view<SteeringAgent, SteeringRequest, Transform, VoxelBody>();

    for (auto [entity, agent, request, transform, body] : view.each()) {
        if (!agent.active) continue;

        if (body.type != VoxelBody::DYNAMIC) continue;

        if (request.mode == SteeringMode::NONE) continue;

        glm::vec3 steering = calculate_force(agent, request, transform, body, time.delta_time);

        // Clamp steering acceleration
        float len = glm::length(steering);
        if (len > agent.max_force) steering = (steering / len) * agent.max_force;

        // Apply steering as acceleration
        body.velocity += steering * time.delta_time;

        // Clamp max speed
        float speed = glm::length(body.velocity);
        if (speed > agent.max_speed) body.velocity = (body.velocity / speed) * agent.max_speed;

        check_completion(entity, request, transform, body);
    }
}

glm::vec3 SteeringSystem::seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& velocity) {
    glm::vec3 desired = glm::normalize(target - pos) * agent.max_speed;

    return desired - velocity;
}

glm::vec3 SteeringSystem::arrive(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, float radius, const glm::vec3& velocity) {
    glm::vec3 to_target = target - pos;
    float dist = glm::length(to_target);

    if (dist < 0.01f) return -velocity;  // brake

    float speed = agent.max_speed;

    if (dist < radius) speed *= (dist / radius);

    glm::vec3 desired = glm::normalize(to_target) * speed;

    return desired - velocity;
}

glm::vec3 SteeringSystem::wander(const SteeringAgent& agent, const glm::vec3& position, const VoxelBody& body, WanderData wander, float dt) {
    // Add random jitter to wanderTarget
    wander.wanderTarget += glm::vec3(
        ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f * wander.wanderJitter * dt, ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f * wander.wanderJitter * dt,
        0.0f
    );  // keep in XY plane

    // Clamp to circle radius
    glm::vec2 xy = glm::vec2(wander.wanderTarget.x, wander.wanderTarget.y);
    if (glm::length2(xy) > wander.wanderRadius * wander.wanderRadius) {
        xy = glm::normalize(xy) * wander.wanderRadius;
        wander.wanderTarget.x = xy.x;
        wander.wanderTarget.y = xy.y;
    }

    // Forward vector in XY
    glm::vec3 forward = body.velocity;
    forward.z = 0.0f;
    if (glm::length2(forward) < 0.0001f)
        forward = glm::vec3(0, 1, 0);
    else
        forward = glm::normalize(forward);

    // Circle center in front of agent
    glm::vec3 circleCenter = position + forward * wander.wanderDistance;

    // Final target
    glm::vec3 targetPos = circleCenter + wander.wanderTarget;

    // Compute steering force
    glm::vec3 toTarget = targetPos - position;
    if (glm::length2(toTarget) < 0.0001f) return glm::vec3(0.0f);

    glm::vec3 desired = glm::normalize(toTarget) * agent.max_speed;

    // Steering = desired - current velocity
    return desired - body.velocity;
}

void SteeringSystem::check_completion(Entity entity, SteeringRequest& request, const Transform& transform, VoxelBody& body) {
    if (request.mode != SteeringMode::ARRIVE) return;

    float dist = glm::distance(transform.get_world_position(), request.target_position);

    float speed = glm::length(body.velocity);

    // Consider both distance & speed
    if (dist < request.arrive_radius && speed < 0.1f) {
        request.completed = true;
        request.mode = SteeringMode::NONE;

        // stop the body
        body.velocity = glm::vec3(0);
        body.angular_velocity = glm::vec3(0);
    }
}

glm::vec3 SteeringSystem::calculate_force(const SteeringAgent& agent, const SteeringRequest& request, const Transform& transform, const VoxelBody& body, float dt) {
    glm::vec3 position = transform.get_world_position();

    switch (request.mode) {
        case SteeringMode::SEEK:
            return seek(agent, position, request.target_position, body.velocity);

        case SteeringMode::ARRIVE:
            return arrive(agent, position, request.target_position, request.arrive_radius, body.velocity);

        case SteeringMode::FLEE:
            return -seek(agent, position, request.target_position, body.velocity);

        case SteeringMode::WANDER:
            return wander(agent, position, body, request.wanderData, dt);

        default:
            return glm::vec3(0);
    }
}

}  // namespace tmt
