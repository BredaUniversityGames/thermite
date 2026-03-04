#pragma once

#include <glm/glm.hpp>

#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"

#include "components/steering_agent.hpp"
#include "components/steering_mode.hpp"

namespace tmt {

class SteeringSystem : public ISystem {
   public:
    // Inherited via ISystem
    std::string get_name() override { return "Steering System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    glm::vec3 seek(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target);
    glm::vec3 arrive(const SteeringAgent& agent, const glm::vec3& pos, const glm::vec3& target, float radius);

    void check_completion(Entity entity, SteeringAgent& agent, SteeringRequest& request, Transform& transform) {
      /*  if (request.mode == SteeringMode::Arrive) {
            float dist = glm::distance(transform.position, request.target_position);

            if (dist < request.arrive_radius) {
                request.completed = true;
                request.mode = SteeringMode::None;
                agent.velocity = {};
            }
        }*/
    }
};

}  // namespace tmt 
