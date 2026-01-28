#include "colorspace.hpp"

namespace tmt::cs {

/* clang-format off */

float linearize_srgb(const float value) {
    if (value <= 0.04045f) {
        return value / 12.92f;
    } else {
        return powf((value + 0.055f) / 1.055f, 2.4f);
    }
}

glm::vec3 linearize_srgb(const glm::vec3 nonlinear) {
    /* Non-linear SRGB (Rec.709) -> Linear SRGB (Rec.709) */
    const glm::vec3 linear_srgb = glm::vec3(
        linearize_srgb(nonlinear.r),
        linearize_srgb(nonlinear.g),
        linearize_srgb(nonlinear.b)
    );
    return linear_srgb;
}

float delinearize_srgb(const float value) {
    if (value <= 0.018f) {
        return value * 4.5f;
    } else {
        return 1.099f * powf(value, 0.45f) - 0.099f;
    }
}

glm::vec3 delinearize_srgb(const glm::vec3 linear) {
    /* Linear SRGB (Rec.709) -> Non-linear SRGB (Rec.709) */
    const glm::vec3 nonlinear_srgb = glm::vec3(
        delinearize_srgb(linear.r),
        delinearize_srgb(linear.g),
        delinearize_srgb(linear.b)
    );
    return nonlinear_srgb;
}

glm::vec3 aces_filmic_tone_curve(const glm::vec3 acescg) {
    /* Linear ACEScg -> Linear ACES2065-1 */
    const glm::vec3 aces2065 = glm::vec3(
         0.695452241f * acescg.r + 0.140678697f * acescg.g + 0.163869062f * acescg.b,
         0.044794563f * acescg.r + 0.859671118f * acescg.g + 0.095534319f * acescg.b,
        -0.005525882f * acescg.r + 0.004025210f * acescg.g + 1.001500672f * acescg.b
    );

    /* Tonemapping */
    constexpr float a = 2.51f, b = 0.03f;
    constexpr float c = 2.43f, d = 0.59f, e = 0.14f;
    const glm::vec3 tonemapped = glm::vec3(
        glm::max(0.0f, (aces2065.r * (a * aces2065.r + b)) / (aces2065.r * (c * aces2065.r + d) + e)),
        glm::max(0.0f, (aces2065.g * (a * aces2065.g + b)) / (aces2065.g * (c * aces2065.g + d) + e)),
        glm::max(0.0f, (aces2065.b * (a * aces2065.b + b)) / (aces2065.b * (c * aces2065.b + d) + e))
    );

    /* Linear ACES2065-1 -> Linear ACEScg */
    return glm::vec3(
         1.451439316f * aces2065.r + -0.236510747f * aces2065.g + -0.214928569f * aces2065.b,
        -0.076553773f * aces2065.r +  1.176229700f * aces2065.g + -0.099675927f * aces2065.b,
         0.008316148f * aces2065.r + -0.006032450f * aces2065.g +  0.997716302f * aces2065.b
    );
}

glm::vec3 linear_srgb_to_acescg(const glm::vec3 linear_srgb) {
    /* Linear SRGB (Rec.709) -> Linear ACEScg */
    const glm::vec3 linear_acescg = glm::vec3(
        0.613132422390542f * linear_srgb.r + 0.339538015060644f * linear_srgb.g + 0.047329562519714f * linear_srgb.b,
        0.070124380833917f * linear_srgb.r + 0.916394011313573f * linear_srgb.g + 0.013451607852510f * linear_srgb.b,
        0.020587657528185f * linear_srgb.r + 0.109574571610682f * linear_srgb.g + 0.869837770861133f * linear_srgb.b
    );
    return linear_acescg;
}

glm::vec3 acescg_to_linear_srgb(const glm::vec3 acescg) {
    /* Linear ACEScg -> Linear SRGB (Rec.709) */
    return glm::vec3(
        glm::max(0.0f,  1.705050993f * acescg.r - 0.621792120f * acescg.g - 0.083258873f * acescg.b),
        glm::max(0.0f, -0.130256414f * acescg.r + 1.140804737f * acescg.g - 0.010548323f * acescg.b),
        glm::max(0.0f, -0.024003530f * acescg.r - 0.128968719f * acescg.g + 1.152972249f * acescg.b)
    );
}

glm::vec3 srgb_to_acescg(const glm::vec3 srgb) {
    /* Non-linear SRGB (Rec.709) -> Linear SRGB (Rec.709) */
    const glm::vec3 linear_srgb = glm::vec3(
        linearize_srgb(srgb.r),
        linearize_srgb(srgb.g),
        linearize_srgb(srgb.b)
    );

    /* Linear SRGB (Rec.709) -> Linear ACEScg */
    const glm::vec3 linear_acescg = glm::vec3(
        0.613132422390542f * linear_srgb.r + 0.339538015060644f * linear_srgb.g + 0.047329562519714f * linear_srgb.b,
        0.070124380833917f * linear_srgb.r + 0.916394011313573f * linear_srgb.g + 0.013451607852510f * linear_srgb.b,
        0.020587657528185f * linear_srgb.r + 0.109574571610682f * linear_srgb.g + 0.869837770861133f * linear_srgb.b
    );
    return linear_acescg;
}

glm::vec3 acescg_to_srgb(const glm::vec3 acescg) {
    return delinearize_srgb(acescg_to_linear_srgb(acescg));
}

/* Transformation matrix going from XYZ space to linear RGB space. */
constexpr glm::mat3x3 xyz_to_rgb = {3.2404542f,  -0.9692660f, 0.0556434f, -1.5371385f, 1.8760108f,
                                   -0.2040259f, -0.4985314f, 0.0415560f, 1.0572252f};

/* Convert temperature in kelvin to a linear SRGB color. */
/* Equations from <https://google.github.io/filament/Filament.html#lighting> */
glm::vec3 temperature_to_linear_srgb(const float t) {
    const float t2 = t * t;

    const float u_num = 0.860117757f + 1.54118254e-4f * t + 1.28641212e-7f * t2;
    const float u_den = 1.0f + 8.42420235e-4f * t + 7.08145163e-7f * t2;
    const float u = u_num / u_den;

    const float v_num = 0.317398726f + 4.22806245e-5f * t + 4.20481691e-8f * t2;
    const float v_den = 1.0f - 2.89741816e-5f * t + 1.61456053e-7f * t2;
    const float v = v_num / v_den;

    const float xy_den = 2.0f * u - 8.0f * v + 4.0f;
    const float x = (3.0f * u) / xy_den;
    const float y = (2.0f * v) / xy_den;

    const float cx = x / y;
    const float cz = (1.0f - x - y) / y;

    /* Return the normalized linear color */
    const glm::vec3 linear = xyz_to_rgb * glm::vec3(cx, 1.0f, cz);
    const float max_linear = glm::max(glm::max(linear.r, linear.g), linear.b);
    return linear / max_linear;
}

/* clang-format on */

}  // namespace tmt::cs
