import view;
import math;
import voxel;
import bvh2;
import material;

import bindless;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Render target. */
RWTexture2D<float4> render_target;

/** @brief 3D PCG hash function. */
inline uint3 pcg3d(const uint3 seed) {
    const uint3 state = seed * 747796405u + 2891336453u;
    const uint3 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

/** @brief Generate a random color based on a seed. */
inline float3 color(const uint seed) {
    const uint3 r = pcg3d(uint3(seed, seed << 12, seed << 24));
    return 0.25 + 0.75 * ((float3)r * (1.0 / float(0xffffffffu)));
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int group_id: SV_GroupIndex) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Trace the scene BVH */
    const Hit hit = trace_bvh2<true>(bvh_nodes, object_indices, object_data, ray);
    // const float hit_t = trace_svt64(blas, voxels, ray, group_id);
    // render_target[thread_id] = float4((hit.t * 0.1f).xxx, 1.0f);

    // render_target[thread_id] = float4(hit.t == 1e30f ? 0.0f : hit.debug, 1.0f);
    // if ((hit.t % (UNITS_PER_VOXEL * 2.0f)) < UNITS_PER_VOXEL) {
    //     render_target[thread_id] = float4(1.0f, 0.0f, 0.0f, 1.0f);
    // } else {
    //     render_target[thread_id] = float4(0.0f, 0.0f, 1.0f, 1.0f);
    // }
    // const float total_distance = hit.t * 0.1f;
    // const float voxel_distance = (hit.t * UNITS_PER_VOXEL) % 1.0f;
    // render_target[thread_id] = float4(total_distance, 0.0f, voxel_distance, 1.0f);
    if (hit.t < BIG_F32) {
        // const VoxelObject object = object_data[hit.object_index];
        // const StructuredBuffer<uint> voxels = bindless::storage_buffer<StructuredBuffer<uint>>(object.voxels_handle);
        // const StructuredBuffer<Material> palette = bindless::storage_buffer<StructuredBuffer<Material>>(object.palette_handle);

        // const uint palette_index = (voxels[hit.voxel_index >> 2u] >> ((4u - (hit.voxel_index % 4u)) << 3u)) & 0xFFu;
        // const float3 albedo = palette[palette_index].albedo;
        // render_target[thread_id] = float4(albedo, 1.0f);
        render_target[thread_id] = float4(color(voxel_hash(hit.voxel_index, hit.object_index, hit.face_index)), 1.0f);

    } else {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
        // const float3 color = heat_color((float)hit.steps / 128.0f);
        // render_target[thread_id] = float4(color, 1.0f);
    }

    // if (hit.t < BIG_F32) {
    //     render_target[thread_id] = float4(hit.albedo, 1.0f);
    // } else {
    //     render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    // }
    
    // render_target[thread_id] = float4((hit.t * 0.01f).xxx, 1.0f);
    // if (hit.t < BIG_F32) {
    //     render_target[thread_id] = float4(1.0f - (hit.t * 0.01f).xxx, 1.0f);
    // } else {
        // const float3 color = heat_color((float)hit.steps / 128.0f);
        // render_target[thread_id] = float4(color, 1.0f);
    // }
}
