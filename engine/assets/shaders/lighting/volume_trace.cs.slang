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

/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Intermediate output buffer. */
RWTexture2D<float3> intermediate_buffer;

[[vk::push_constant]] int cascade_index;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Center position of the cascade hierarchy */
    const float3 cascade_center = 0.0f;

    /* Cascade size increases 2x with each subsequent cascade */
    const float c_half_extent = float(CASCADE_HALF_EXTENT << cascade_index);
    const float c_extent = float(CASCADE_EXTENT << cascade_index);

    /* Calculate the cascade min bounds and probe extent */
    const float3 cascade_min = cascade_center - c_half_extent;
    const float probe_extent = c_extent / CASCADE_WIDTH;

    /* Find the world origin of the probe */
    const float3 probe_origin = cascade_min + probe_extent * thread_probe(thread_id);
    const float2 ray_uv = thread_ray(thread_id) / PROBE_WIDTH;
    const Ray ray = custom_ray(probe_origin, oct_decode(ray_uv));

    /* Trace a ray */
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, ray, BIG_F32);
    const float3 luminance = select(t < BIG_F32, 0.0f, 1.0f);
    intermediate_buffer[thread_texel(thread_id, 0)] = luminance;
}
