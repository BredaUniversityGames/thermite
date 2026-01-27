#pragma once

namespace tmt::cs {

/* Transform color from non-linear SRGB to linear SRGB color-space. (Rec.709) */
float linearize_srgb(const float value);
/* Transform color from non-linear SRGB to linear SRGB color-space. (Rec.709) */
glm::vec3 linearize_srgb(const glm::vec3 nonlinear);

/* Transform color from linear SRGB to non-linear SRGB color-space. (Rec.709) */
float delinearize_srgb(const float value);
/* Transform color from linear SRGB to non-linear SRGB color-space. (Rec.709) */
glm::vec3 delinearize_srgb(const glm::vec3 linear);

/* ACES tonemapping curve (Narkowicz 2015) */
glm::vec3 aces_filmic_tone_curve(const glm::vec3 aces2065);

/* Transform color from linear SRGB (Rec.709) to linear ACEScg color-space. */
glm::vec3 linear_srgb_to_acescg(const glm::vec3 linear_srgb);
/* Transform color from linear ACEScg to linear SRGB (Rec.709) color-space and apply tonemapping. */
glm::vec3 acescg_to_linear_srgb(const glm::vec3 acescg);

/* Transform color from non-linear SRGB (Rec.709) to linear ACEScg color-space. */
glm::vec3 srgb_to_acescg(const glm::vec3 srgb);
/* Transform color from linear ACEScg to non-linear SRGB (Rec.709) color-space and apply tonemapping. */
glm::vec3 acescg_to_srgb(const glm::vec3 acescg);

/* Convert temperature in kelvin to a linear SRGB (Rec.709) color. */
glm::vec3 temperature_to_linear_srgb(const float t);

}  // namespace tmt
