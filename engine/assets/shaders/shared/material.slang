/**
 * @name Thermite Material Model.
 * This file contains the implementation for our material model based on OpenPBR.
 * Our shading frame is Z-up.
 */
module material;

import math;

public struct ShadingFrame {
    public float3 wi;
    public float3 wo;
    public float3 wh;
};

/* Create a shading frame for the given normal, incident and outgoing directions. */
public [ForceInline] ShadingFrame create_shading_frame(const float3 n, const float3 wi, const float3 wo) {
    const Basis basis = orthonormal_basis(n);
    const float3 t = cross(n, float3(0.0f, 1.0f, 0.0f));
    const float3 b = cross(n, t);
    const float3x3 world_to_shade = float3x3(t, b, n);
    const float3 wi_shade = mul(world_to_shade, wi);
    const float3 wo_shade = mul(world_to_shade, wo);
    return ShadingFrame(wi_shade, wo_shade, normalize(wi_shade + wo_shade));
}

/* Voxel material data, used in a palette. */
public struct Material {
    /* `Base Color` Overall color of the material used for diffuse, metal and transmission. */
    public float3 albedo;
    // /* `Roughness` Specifies microfacet roughness of the surface for specular reflection and transmission. */
    // public float roughness;
    // /* `Emission Color` Color of light emitted from the surface. */
    // public float3 emission_color;
    // /* `Emission` Strength of the emitted light. */
    // public float emission;
    // /* `Specular Tint` Color tint for specular and metallic reflection. */
    // public float3 specular_tint;
    // /* `Specular` Adjustment to the IOR to increase or decrease intensity of the specular layer. */
    // /* 0.5 means no adjustment, 0 removes all reflections, 1 doubles them at normal incidence. */
    // public float specular;
    // /* `Metallic` Blends between a dielectric and metallic material model. */
    // public float metallic;
    // /* `IOR` Index of refraction (IOR) for specular reflection and transmission. */
    // public float ior;
    // /* `Transmission` Transmission is used to render materials like glass and liquids, */
    // /* where the surface both reflects light and transmits it into the interior of the object. */
    // public float transmission;
};

/* f_layer(ωi,ωo) = f_coat(ωi,ωo) + (1 - E_coat(ωo)) * f_sub(ωi,ωo) */
/* <https://academysoftwarefoundation.github.io/OpenPBR/#layering> */
[ForceInline] float3 f_layer(const float3 fsub, const float3 fcoat) {
    /* Do we need "directional albedo integral" here? (7) */
    /* <https://pbr-book.org/4ed/Reflection_Models/BSDF_Representation#HemisphericalReflectance> */
    return fcoat + fsub;
}

/* f_mix = (1 - w1) * f0 + w1 * f1 = lerp(f0,f1,w1) */
/* <https://academysoftwarefoundation.github.io/OpenPBR/#mixing> */
[ForceInline] float3 f_mix(const float3 f0, const float3 f1, const float w1) {
    return (1.0f - w1) * f0 + w1 * f1;
}

/*******************************************************************/
/*                                                                 */
/*  ~ thermite material model                        emission      */
/*                                                      ^          */
/*    ambient medium                                    |          */
/*  +---------------------+---------------------+-------+-------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |     gloss     |  */
/*  |░░░░░░░ metal ░░░░░░░|     translucent     |               |  */
/*  |░░░░░░░░░░░░░░░░░░░░░|        base         +---------------+  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░diffuse░░░░|  */
/*  |░░░░░░░░░░░░░░░░░░░░░|                     |░░░░░░░░░░░░░░░|  */
/*  + - - - - - - - - - - +- - - - - - - - - - -+ - - - - - - - +  */
/*                                              <- opaque-base ->  */
/*                        <---------- dielectric-base ---------->  */
/*  <--------------------------- base -------------------------->  */
/*                                                                 */
/*******************************************************************/

/* Lambertian BRDF. */
/* Source: https://pbr-book.org/4ed/Reflection_Models */
[ForceInline] float3 diffuse_slab(const float3 albedo) {
    return albedo * RCP_PI_F32;
}

/* Torrance–Sparrow Microfacet BRDF. */
/* Source: https://www.pbr-book.org/4ed/Reflection_Models */ 
[ForceInline] float gloss_slab(
    const float wi_costheta, /* incident cos(theta) */
    const float wo_costheta, /* outgoing cos(theta) */
    const float wh_costheta, /* half-way cos(theta) */
    const float alpha_2, float eta
) {
    /* Trowbridge-Reitz normal distribution function */
    const float wh_costheta_2 = wh_costheta * wh_costheta;
    const float wh_costheta_4 = wh_costheta_2 * wh_costheta_2;
    const float wh_sintheta_2 = 1.0f - wh_costheta_2;
    const float wh_tantheta_2 = wh_sintheta_2 / wh_costheta_2;
    const float e = alpha_2 + wh_tantheta_2;
    const float d = alpha_2 / (PI_F32 * wh_costheta_4 * e * e);

    /* Trowbridge-Reitz masking and shadowing function */
    /* NOTE: division by zero here */
    const float wi_tantheta = abs((1.0f - wi_costheta) / wi_costheta);
    const float wi_lambda = (-1.0f + sqrt(1.0f + alpha_2 * wi_tantheta * wi_tantheta)) * 0.5f;
    const float wo_tantheta = abs((1.0f - wo_costheta) / wo_costheta);
    const float wo_lambda = (-1.0f + sqrt(1.0f + alpha_2 * wo_tantheta * wo_tantheta)) * 0.5f;
    const float g = 1.0f / (1.0f + wi_lambda + wo_lambda);

    float i_costheta = wi_costheta;
    /* If the incoming direction comes from within */
    if (i_costheta < 0.0f) {
        /* Invert relative index of refraction, ensuring cos(theta_i) is always positive */
        eta = rcp(eta);
        i_costheta = -i_costheta;
    }

    /* Apply Snell's law to find cos(theta_t) */
    const float i_sintheta_2 = 1.0f - i_costheta * i_costheta;
    const float t_sintheta_2 = i_sintheta_2 / (eta * eta);
    const bool t_int = t_sintheta_2 >= 1.0f; /* Total internal reflection */
    const float t_costheta = sqrt(1.0f - t_sintheta_2);

    /* Fresnel equation */
    const float r_parl = (eta * i_costheta - t_costheta) / (eta * i_costheta + t_costheta);
    const float r_perp = (i_costheta - eta * t_costheta) / (i_costheta + eta * t_costheta);
    const float f = 1.0f; // t_int ? 1.0f : ((r_parl * r_parl + r_perp * r_perp) * 0.5f);

    /* Torrance–Sparrow BRDF */
    return (d * g * f) / (4.0f * wo_costheta * wi_costheta);
}

/* Evaluate opaque base material slab. (diffuse & gloss) */
[ForceInline] public float3 opaque_base_slab(
    const float wi_costheta, const float wo_costheta, 
    const float wh_costheta, const float3 albedo, 
    const float alpha, const float eta
) {
    /* Evaluate the two slabs */
    const float3 diffuse = diffuse_slab(albedo);
    const float3 gloss = gloss_slab(wi_costheta, wo_costheta, wh_costheta, alpha * alpha, eta);

    /* Layer the two slabs, diffuse = substrate, gloss = coating */
    return f_layer(diffuse, gloss);
}
