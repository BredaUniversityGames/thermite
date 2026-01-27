#include "light.hpp"

#include <glm/ext/scalar_constants.hpp>

#include "engine/shared/colorspace.hpp"

namespace tmt {

glm::vec3 Light::calculate_luminance() const {
    /* Calculate the linear ACEScg temperture color */
    const glm::vec3 temperature_linear_srgb = cs::temperature_to_linear_srgb(temperature);
    const glm::vec3 temperature_acescg = cs::linear_srgb_to_acescg(temperature_linear_srgb);

    switch (type) {
        /* Spherical area light */
        case LightType::SPHERE_LIGHT: {
            const tmt::SphereLight sphere_light = std::get<tmt::SphereLight>(light);
            const float luminous_intensity = sphere_light.luminous_flux / (glm::pi<float>() * 4.0f);
            return color * temperature_acescg * luminous_intensity;
        }
    }

    return glm::vec3(100.0f, 0.0f, 100.0f);
}

}  // namespace tmt
