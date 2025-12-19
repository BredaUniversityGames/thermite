#pragma once

namespace tmt {
struct ParticleEmitter {
    glm::vec4 start_color = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 end_color = {1.0f, 0.0f, 0.0f, 0.0f};
    
    glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    float particle_lifetime = 1.0f;

    glm::vec3 velocity_min = {0.0f, 0.0f, 0.0f};
    float velocity_max = 10.0f;
    uint32_t spawn_count = 8u;

    bool active = true;
};
}  // namespace tmt

TMT_COMPONENT(tmt::ParticleEmitter, "ParticleEmitter", (start_color, end_color, pos, particle_lifetime, velocity_min, velocity_max, spawn_count, active));