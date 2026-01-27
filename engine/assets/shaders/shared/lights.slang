module lights;

import colorspace;

public enum LightType : uint {
    SPHERE_LIGHT,
    SUN_LIGHT,
    SPOT_LIGHT,
    TUBE_LIGHT,
};

/* Universal light descriptor. */
public struct Light {
    float4 type_origin;
    float4 luminance_source;
    float4 attenuation_culling_blend;
    float4 direction_length;

    /* The kind of light source. */
    public property LightType type { get { return (LightType)asuint(type_origin.x); } }
    /* Origin point of the light source. */
    public property float3 origin { get { return type_origin.yzw; } }
    /* Exitent luminance of the light source. (in candela, in ACEScg color-space) */
    public property float3 luminance { get { return luminance_source.xyz; } }
    /* Radius of the light source, defines the softness of its shadows. */
    public property float source_radius { get { return luminance_source.w; } }
    /* Radius/distance at which the emitted light influence reaches zero. */
    public property float attenuation_radius { get { return attenuation_culling_blend.x; } }
    /* Radius/distance at which the light source should be culled. */
    public property float culling_radius { get { return attenuation_culling_blend.y; } }
    /* Cosine of the angular radius of the spot light beam. */
    public property float beam_angle { get { return attenuation_culling_blend.z; } }
    /* The softness of the spot light edge. (0..1) */
    public property float spot_blend { get { return attenuation_culling_blend.w; } }
    /* Primary direction of the light source. */
    public property float3 direction { get { return direction_length.xyz; } }
    /* Length of the light source. (used for tube lights) */
    public property float source_length { get { return direction_length.w; } }
};

/* Calculate distance attenuation. */
public [ForceInline] float distance_attenuation(const float dist, const float radius) {
    const float relative_dist = saturate(dist / radius);
    const float inverse_dist = 1.0f - relative_dist * relative_dist;
    return (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);
}

/* For a given sample point and normal, evaluate the weight of a light source. */
public [ForceInline] float evaluate_weight(const float3 p, const float3 n, const Light light) {
    /* Calculate the local origin and distance to the light source */
    const float3 src_local = light.origin - p;
    const float src_dist = length(src_local);
    if (src_dist > light.culling_radius) return 0.0f; /* Out of range */

    /* Calculate the direction and angle towards the light source */
    const float3 src_dir = src_local / src_dist;
    const float src_angle = dot(src_dir, -light.direction);
    if (src_angle < light.beam_angle) return 0.0f; /* Out of range */

    /* Calculate light attenuation */
    const float ndotl = max(0.0f, dot(n, src_dir));
    const float luminance = cs::acescg_luminance(light.luminance);
    return luminance * ndotl * distance_attenuation(src_dist, light.attenuation_radius);
}
