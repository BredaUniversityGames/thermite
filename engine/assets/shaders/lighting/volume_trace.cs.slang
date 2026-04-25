/**
 * @name Irradiance Volume Update [1 thread per ray]
 * 64 Rays per probe, 16x16x16 Probes per cascade, 6 Cascades.
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import voxel;
import bvh2;
import volume;
import lights;
import lightgrid;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Lights buffer. */
StructuredBuffer<Light> lights;
/* Light grid buffer. */
StructuredBuffer<uint32_t> light_grid;
/* Intermediate output buffer. */
RWTexture2D<float3> intermediate_buffer;
RWTexture2D<float3> dir_buffer;

[[vk::push_constant]] int cascade_index;

#define EARLY_OUT { intermediate_buffer[atlas_texel] = 0.0f; dir_buffer[atlas_texel] = 0.0f; return; }

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Center position of the cascade hierarchy */
    const float3 cascade_center = floor(render_view.origin.xyz / 2.0f) * 2.0f;

    /* Cascade size increases 2x with each subsequent cascade */
    const float c_half_extent = float(iv::CASCADE_HALF_EXTENT << cascade_index);
    const float c_extent = float(iv::CASCADE_EXTENT << cascade_index);

    /* Calculate the cascade min bounds and probe extent */
    const float3 cascade_min = cascade_center - c_half_extent;
    const float probe_extent = c_extent / iv::CASCADE_WIDTH;
    const float3 probe_origin = cascade_min + probe_extent * iv::thread_probe(thread_id);
    const int2 atlas_texel = iv::thread_texel(thread_id, 0);

    /* Generate a seed for this thread */
    uint seed = thread_id.x + thread_id.y * iv::ATLAS_SIZE.x + (render_view.frame_index * (iv::ATLAS_SIZE.x * (iv::ATLAS_SIZE.y / iv::CASCADE_COUNT)));

    /* Pick a light using the cascaded light grid */
    const lgrid::PickedLight picked_light = lgrid::pick_light(light_grid, probe_origin - render_view.origin.xyz, seed);
    if (picked_light.index == uint::maxValue) EARLY_OUT;
    const Light light = lights[picked_light.index];

    /* Generate a sample for the light we picked */
    const LightSample light_sample = sample_light(probe_origin, light, rand_float2(seed));
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(probe_origin, light_sample.dir), light_sample.tmax);
    if (t < light_sample.tmax) EARLY_OUT;

    /* Light attenuation */
    const float attenuation = attenuate_light(probe_origin, light, light_sample);

    /* Calculate light illuminance */
    intermediate_buffer[atlas_texel] = light.luminance * attenuation * picked_light.rcp_pdf * light_sample.rcp_pdf;
    dir_buffer[atlas_texel] = light_sample.dir;

    /* Find the world origin of the probe */
    // const float3 probe_origin = cascade_min + probe_extent * thread_probe(thread_id);
    // const float2 ray_uv = thread_ray(thread_id) / PROBE_WIDTH;
    // const Ray ray = custom_ray(probe_origin, oct_decode(ray_uv));

    /* Trace a ray */
    // const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, ray, BIG_F32);
    // const float3 luminance = select(t < BIG_F32, 0.0f, 1.0f);
    // intermediate_buffer[thread_texel(thread_id, 0)] = luminance;
}
