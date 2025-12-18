#pragma once

namespace tmt {
struct ParticleEmitter {
    glm::vec4 start_color = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 end_color = {1.0f, 0.0f, 0.0f, 0.0f};
    
    glm::vec3 start_pos = {0.0f, 0.0f, 0.0f};
    float particle_lifetime = 1.0f;

    glm::vec3 end_pos = {0.0f, 0.0f, 0.0f};
    uint32_t particles = 8u;
};
}  // namespace tmt