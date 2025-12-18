#pragma once

namespace tmt {
struct ParticleEmitter {
    glm::vec4 color = {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 spawn_pos = {0.0f, 0.0f, 0.0f};
};
}  // namespace tmt