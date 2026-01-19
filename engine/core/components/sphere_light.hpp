#pragma once

#include <glm/glm.hpp>

#include "engine/core/reflection.hpp"

namespace tmt {

/* Spherical area light component. */
struct SphereLight {
    /* Radius of the sphere light source. (larger means softer shadows) */
    float source_radius = 0.2f; 
    /* Radius at which the light influence will be zero. */
    float attenuation_radius = 10.0f; 

    /* Color of the light emitted. */
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    /* Luminous flux of the spot light. (in lumen, default: 100lm) */
    float luminous_flux = 100.0f;
    /* Temperature of the light emitted. (in kelvin, default: 6500k) */
    float temperature = 6500.0f;
};

}  // namespace tmt

TMT_COMPONENT(tmt::SphereLight, "Sphere Light", (source_radius, attenuation_radius, color, luminous_flux, temperature));
