/**
 * @name Irradiance Volume Update [1 thread per ray]
 * 64 Rays per probe, 16x16x16 Probes per cascade, 6 Cascades.
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import voxel;
import bvh2;
import cache;

// /* Active render & scene view. */
// ConstantBuffer<RenderView> render_view;
// ConstantBuffer<SceneView> scene_view;
// Texture2D<float4> blue_noise;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
// /* Macrofacet cache buffer. */
// RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Intermediate output buffer. */
RWTexture3D<float3> intermediate_buffer;

[[vk::push_constant]] uint cascade_index;

/* Octahedral probe patch size in pixels. */
const static int2 PROBE_SIZE = int2(8, 8); /* px */
const static int C_WIDTH = 16;
const static uint C0_EXTENT = 32u; /* u */
const static uint C0_HALF_EXTENT = C0_EXTENT >> 1;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int3 thread_id: SV_DispatchThreadID, int3 ray_id: SV_GroupThreadID, int3 probe_id: SV_GroupID) {
    /* Center position of the cascade hierarchy */
    const float3 cascade_center = 0.0f;

    /* Cascade size increases 2x with each subsequent cascade */
    const float c_half_extent = float(C0_HALF_EXTENT << cascade_index);
    const float c_extent = float(C0_EXTENT << cascade_index);

    /* Calculate the cascade min bounds and probe extent */
    const float3 cascade_min = cascade_center - c_half_extent;
    const float probe_extent = c_extent / C_WIDTH;

    // /* Find the world origin of the probe */
    const float3 probe_origin = cascade_min + float3(probe_extent) * (float3(probe_id) + 0.5f);
    const float2 ray_uv = (float2(ray_id.xy) + 0.5f) / PROBE_SIZE;
    const Ray ray = custom_ray(probe_origin, oct_decode(ray_uv));

    // /* Trace a ray */
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, ray, BIG_F32);
    const float3 luminance = select(t < BIG_F32, 0.0f, 1.0f);

    intermediate_buffer[thread_id] = luminance;
}
