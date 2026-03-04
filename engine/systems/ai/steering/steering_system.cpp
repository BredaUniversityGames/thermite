#include "steering_system.hpp"

#include "core/logger.hpp"

namespace tmt {

void SteeringSystem::on_start() {
    Log::info("Steering on_start");
}

void SteeringSystem::on_update(const FrameData& time) {
    //auto view = engine.ecs.view<SteeringAgent, SteeringRequest, Transform>();

    //for (auto [entity, agent, request, transform] : view.each()) {
    //    if (!agent.active || request.mode == SteeringMode::None) continue;

    //    glm::vec3 force = calculate_force(agent, request, transform);

    //    // Apply physics
    //    agent.acceleration = force / agent.mass;
    //    agent.velocity += agent.acceleration * time.delta_time;

    //    if (glm::length(agent.velocity) > agent.max_speed) agent.velocity = glm::normalize(agent.velocity) * agent.max_speed;

    //    transform.position += agent.velocity * time.delta_time;

    //    check_completion(entity, agent, request, transform);
    //}
}

glm::vec3 SteeringSystem::seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target) {
    glm::vec3 desired = glm::normalize(target - pos) * agent.max_speed;
    glm::vec3 steer = desired - agent.velocity;
    return glm::clamp(steer, -agent.max_force, agent.max_force);
}

glm::vec3 SteeringSystem::arrive(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, float radius) {
    glm::vec3 to_target = target - pos;
    float dist = glm::length(to_target);

    if (dist < 0.01f) return glm::vec3(0);

    float speed = agent.max_speed;

    if (dist < radius) speed *= (dist / radius);

    glm::vec3 desired = glm::normalize(to_target) * speed;
    return desired - agent.velocity;
}

}  // namespace tmt