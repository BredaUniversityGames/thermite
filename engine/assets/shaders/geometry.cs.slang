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
    }
}
