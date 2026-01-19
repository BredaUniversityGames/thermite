#pragma once

#include <glm/glm.hpp>

#include "engine/core/reflection.hpp"

namespace tmt {

/* Spot light component. */
struct SpotLight {
    /* Radius of the spot light source. (larger means softer shadows) */
    float source_radius = 0.2f; 
    /* Distance at which the light influence will be zero. */
    float attenuation_distance = 10.0f; 
    /* Angular diameter of the spot light beam. (in radians, default: 45deg) */
    float beam_half_angle = 0.785398f;
    /* The softness of the spot light edge. (0..1, default: 20%) */
    float spot_blend = 0.2f;

    /* Color of the light emitted. */
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    /* Luminous flux of the spot light. (in lumen, default: 100lm) */
    float luminous_flux = 100.0f;
    /* Temperature of the light emitted. (in kelvin, default: 6500k) */
    float temperature = 6500.0f;
};

}  // namespace tmt

TMT_COMPONENT(tmt::SpotLight, "Spot Light", (source_radius, attenuation_distance, beam_half_angle, spot_blend, color, luminous_flux, temperature));
