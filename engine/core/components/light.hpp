#pragma once

#include <variant>
#include <glm/glm.hpp>

#include "engine/core/reflection.hpp"

namespace tmt {

enum class LightType : uint32_t {
    /* Spherical area light. */
    SPHERE_LIGHT = 0u,
    /* Directional area light. */
    DIR_LIGHT = 1u,
    /* Spot light. */
    SPOT_LIGHT = 2u,
    /* Tube area light. */
    TUBE_LIGHT = 3u,
};

/* Spherical area light. */
struct SphereLight {
    /* Radius of the sphere light source. (larger means softer shadows) */
    float source_radius = 0.2f; 
    /* Radius at which the light influence will be zero. */
    float attenuation_radius = 10.0f; 

    /* Luminous flux of the sphere light. (in lumen, default: 100lm) */
    float luminous_flux = 100.0f;
};

/* Directional area light. */
struct DirLight {
    /* Angular diameter of the directional light. (larger means softer shadows) */
    float source_angle = 0.1f;

    /* Luminous intensity of the directional light. (in candela, default: 100cd) */
    float luminous_intensity = 100.0f;
};

/* Spot light. */
struct SpotLight {
    /* Radius of the spot light source. (larger means softer shadows) */
    float source_radius = 0.2f; 
    /* Distance at which the light influence will be zero. */
    float attenuation_distance = 10.0f; 
    /* Angular diameter of the spot light beam. (in radians, default: 45deg) */
    float beam_angle = 0.785398f;
    /* The softness of the spot light edge. (0..1, default: 20%) */
    float spot_blend = 0.2f;

    /* Luminous flux of the spot light. (in lumen, default: 100lm) */
    float luminous_flux = 100.0f;
};

/* Tube area light. */
struct TubeLight {
    /* Radius of the tube light source. (larger means softer shadows) */
    float source_radius = 0.2f; 
    /* Distance at which the light influence will be zero. */
    float attenuation_distance = 10.0f; 

    /* Luminous flux of the tube light. (in lumen, default: 100lm) */
    float luminous_flux = 100.0f;
};

/* Light source component. */
struct Light {
    /* Color of the light emitted. */
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    /* Temperature of the light emitted. (in kelvin, default: 6500k) */
    float temperature = 6500.0f;

    /* Type of the light source. */
    LightType type = LightType::SPHERE_LIGHT;

    /* Light properties. */
    std::variant<SphereLight, DirLight, SpotLight, TubeLight> light {};
};

}  // namespace tmt

/* Light types */
TMT_OBJECT(tmt::SphereLight, (source_radius, attenuation_radius, luminous_flux));
TMT_OBJECT(tmt::DirLight, (source_angle, luminous_intensity));
TMT_OBJECT(tmt::SpotLight, (source_radius, attenuation_distance, beam_angle, spot_blend, luminous_flux));
TMT_OBJECT(tmt::TubeLight, (source_radius, attenuation_distance, luminous_flux));

/* Component */
TMT_COMPONENT(tmt::Light, "Light", (color, temperature, type, light));
