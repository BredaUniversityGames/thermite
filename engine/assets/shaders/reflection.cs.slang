import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;
import lights;
import material;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
Texture2D<float4> blue_noise;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Luminance buffer. */
RWTexture2D<float3> lbuffer;

/* Sample glossy specular lobe using VNDF distribution. */
/* Source: "Sampling Visible GGX Normals with Spherical Caps" */
[ForceInline] float3 sample_vndf(float3 wo, float alpha, float2 r01) {
    const float3 wo_std = normalize(float3(wo.xy * alpha, wo.z));
    const float phi = 2.0f * PI_F32 * r01.x;
    const float z = (1.0f - r01.y) * (1.0f + wo_std.z) + -wo_std.z;
    const float sin_theta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    const float x = sin_theta * cos(phi);
    const float y = sin_theta * sin(phi);
    const float3 wm_std = float3(x, y, z) + wo_std;
    return normalize(float3(wm_std.xy * alpha, wm_std.z));
}

[ForceInline] float pdf_vndf(float wh_costheta, float wo_costheta, float alpha_2) {
    /* Trowbridge-Reitz normal distribution function */
    float wh_costheta_2 = wh_costheta * wh_costheta;
    float wh_costheta_4 = wh_costheta_2 * wh_costheta_2;
    float wh_sintheta_2 = 1.0f - wh_costheta_2;
    float wh_tantheta_2 = wh_sintheta_2 / wh_costheta_2;
    float e = alpha_2 + wh_tantheta_2;
    float d = alpha_2 / (PI_F32 * wh_costheta_4 * e * e);

    /* Trowbridge-Reitz masking and shadowing function */
    float wo_sintheta_2 = 1.0f - wo_costheta * wo_costheta;
    float wo_tantheta_2 = wo_sintheta_2 / (wo_costheta * wo_costheta);
    float wo_lambda = (-1.0f + sqrt(1.0f + alpha_2 * wo_tantheta_2)) * 0.5f;
    float g = 1.0f / (1.0f + wo_lambda);

    return (d * g) / (4.0f * wo_costheta);
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Quarter-rate shading */
    const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
    const int2 pixel_id = (thread_id << 1) + offset;

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel_id);

    /* Unpack visibility buffer data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    const float eta = geo.material.ior() / 1.0f;
    if (eta <= 1.0f) return; /* No reflections */

    const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    const float2 noise = r2(blue_noise[pixel_id % 512].rg, render_view.frame_index % 1024u);

    /* Transform vectors into shading space */
    const float3x3 basis = orthonormal_basis(geo.normal);
    const float3 wo = mul(basis, -ray.dir);

    /* Get the material roughness */
    const float roughness = geo.material.roughness();
    const float alpha = roughness * roughness;

    /* Importance sample specular lobe */
    const float3 wh = sample_vndf(wo, alpha, noise);
    const float3 wi = reflect(-wo, wh);

    /* Ignore directions which lie underneath the horizon */
    if (wi.z <= 0.0f) return;
    const float3 sample_dir = mul(transpose(basis), wi);

    /* Evaluate the BSDF and PDF of the sample */
    const float alpha_2 = alpha * alpha;
    const float rcp_pdf = 1.0f / pdf_vndf(wh.z, wo.z, alpha_2);
    const float bsdf = gloss_slab(wi.z, wo.z, wh.z, alpha_2, eta) * rcp_pdf;

    /* Trace sample direction */
    const Hit hit = trace_bvh2<false>(tlas_nodes, object_indices, objects, custom_ray(hit_point, sample_dir));
    if (hit.t >= BIG_F32) return;

    lbuffer[thread_id] += float3(1.0f, 1.0f, 1.0f) * bsdf;
}
