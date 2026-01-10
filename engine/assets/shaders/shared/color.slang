module color;

/* Transform color from non-linear SRGB to linear SRGB color-space. (Rec.709) */
[ForceInline] float linearize_srgb(const float value) {
    if (value <= 0.04045f) {
        return value / 12.92f;
    } else {
        return pow((value + 0.055f) / 1.055f, 2.4f);
    }
}

/* Transform color from linear SRGB to non-linear SRGB color-space. (Rec.709) */
[ForceInline] float delinearize_srgb(const float value) {
    if (value <= 0.018f) {
        return value * 4.5f;
    } else {
        return 1.099f * pow(value, 0.45f) - 0.099f;
    }
}

/* Transform color from non-linear SRGB (Rec.709) to linear ACEScg color-space. */
public [ForceInline] float3 srgb_to_acescg(const float3 srgb) {
    /* Non-linear SRGB (Rec.709) -> Linear SRGB (Rec.709) */
    const float3 linear_srgb = float3(
        linearize_srgb(srgb.r),
        linearize_srgb(srgb.g),
        linearize_srgb(srgb.b)
    );

    /* Linear SRGB (Rec.709) -> Linear ACEScg */
    const float3 linear_acescg = float3(
        0.613132422390542f * linear_srgb.r + 0.339538015060644f * linear_srgb.g + 0.047329562519714f * linear_srgb.b,
        0.070124380833917f * linear_srgb.r + 0.916394011313573f * linear_srgb.g + 0.013451607852510f * linear_srgb.b,
        0.020587657528185f * linear_srgb.r + 0.109574571610682f * linear_srgb.g + 0.869837770861133f * linear_srgb.b
    );
    return linear_acescg;
}

/* ACES tonemapping curve (Narkowicz 2015) */
[ForceInline] float3 aces_filmic_tone_curve(const float3 aces2065) {
    static const float a = 2.51f;
    static const float b = 0.03f;
    static const float c = 2.43f;
    static const float d = 0.59f;
    static const float e = 0.14f;
    return {
        max(0.0f, (aces2065.r * (a * aces2065.r + b)) / (aces2065.r * (c * aces2065.r + d) + e)),
        max(0.0f, (aces2065.g * (a * aces2065.g + b)) / (aces2065.g * (c * aces2065.g + d) + e)),
        max(0.0f, (aces2065.b * (a * aces2065.b + b)) / (aces2065.b * (c * aces2065.b + d) + e))
    };
}

/* Transform color from linear ACEScg to linear SRGB (Rec.709) color-space and apply tonemapping. */
public [ForceInline] float3 acescg_to_linear_srgb(float3 acescg) {
    /* Linear ACEScg -> Linear ACES2065-1 */
    const float3 aces2065 = float3(
         0.695452241f * acescg.r + 0.140678697f * acescg.g + 0.163869062f * acescg.b,
         0.044794563f * acescg.r + 0.859671118f * acescg.g + 0.095534319f * acescg.b,
        -0.005525882f * acescg.r + 0.004025210f * acescg.g + 1.001500672f * acescg.b
    );

    /* Apply ACES tonemapping in ACES2065-1 color-space */
    const float3 tonemapped = aces_filmic_tone_curve(aces2065);

    /* Linear ACES2065-1 -> Linear SRGB (Rec.709) */
    const float3 linear_srgb = float3(
        max(0.0f,  2.52168619f * tonemapped.r + -1.13413099f * tonemapped.g + -0.38755520f * tonemapped.b),
        max(0.0f, -0.27648212f * tonemapped.r +  1.37271909f * tonemapped.g + -0.09623697f * tonemapped.b),
        max(0.0f, -0.01538516f * tonemapped.r + -0.15298289f * tonemapped.g +  1.16836805f * tonemapped.b)
    );
    return linear_srgb;
}

/* Transform color from linear SRGB to non-linear SRGB color-space. (Rec.709) */
public [ForceInline] float3 delinearize_srgb(const float3 linear) {
    /* Linear SRGB (Rec.709) -> Non-linear SRGB (Rec.709) */
    const float3 nonlinear_srgb = float3(
        delinearize_srgb(linear.r),
        delinearize_srgb(linear.g),
        delinearize_srgb(linear.b)
    );
    return nonlinear_srgb;
}
