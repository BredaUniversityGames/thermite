#include "wander_steering.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

namespace tmt {

void WanderSteering::on_start(Entity agent) {
    time_wandering = 0.f;

    auto& registry = engine.ecs.get_registry();

    // Create a wander steering request
    SteeringRequest request {};
    request.mode = SteeringMode::WANDER;

    // set initial WanderData
    request.wander_data.wander_radius = 2.0f;                // radius of the wander sphere
    request.wander_data.wander_distance = 3.0f;              // distance in front of the agent
    request.wander_data.wander_jitter = 0.5f;                // how much the target jitters per second
    request.wander_data.wander_target = glm::vec3(0, 0, 0);  // start at sphere center

    registry.emplace_or_replace<SteeringRequest>(agent, request);

    time_wandering = 0.f;
}

void WanderSteering::on_tick(Entity /*agent*/, float dt) {
    time_wandering += dt;
}

bool WanderSteering::is_done(Entity /*agent*/) const {
    return time_wandering >= wander_duration;
}

void WanderSteering::on_finished(Entity agent) {
    auto& registry = engine.ecs.get_registry();
    if (registry.any_of<SteeringRequest>(agent)) {
        registry.remove<SteeringRequest>(agent);
    }
}

void WanderSteering::on_interrupt(Entity agent) {
    on_finished(agent);
}

}  // namespace tmt
