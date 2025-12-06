import view;
import math;
import voxel;
import bvh2;
import svt64;

import bindless;

/* Ray AABB intersection function. *(returns BIG_F32 if the ray misses the AABB)* */
inline void intersect_aabb(float3 ro, float3 rrd, float3 bmin, float3 bmax, out float hit_t) {
    const float3 t_to_min = (bmin - ro) * rrd;
    const float3 t_to_max = (bmax - ro) * rrd;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
// StructuredBuffer<Svt64Node> blas_nodes;
// StructuredBuffer<uint8_t> voxel_data;
/* Render target. */
RWTexture2D<float4> render_target;

/* Ray AABB intersection function. */
inline float intersect_aabb(Ray ray, float3 box_min, float3 box_max) {
    const float3 t_to_min = (box_min - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (box_max - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    return t_near > t_far ? BIG_F32 : t_near;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int group_id: SV_GroupIndex) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    const float entry_t = intersect_aabb(ray, 1.0f, 2.0f);

    /* Trace the scene BVH */
    const float hit_t = trace_bvh2(bvh_nodes, object_indices, object_data, ray) * 0.01;
    // const float hit_t = trace_svt64(blas, voxels, ray, group_id);
    // render_target[thread_id] = float4(hit_t, hit_t, hit_t, 1.0f);

    if (hit_t < BIG_F32) {
        render_target[thread_id] = float4(1.0f - (hit_t * 0.25f).xxx, 1.0f);
    } else {
        if (entry_t < BIG_F32) {
            render_target[thread_id] = float4(0.5f, 0.05f, 0.05f, 1.0f);
        } else {
            render_target[thread_id] = float4(0.05f, 0.05f, 0.05f, 1.0f);
        }
    }
}
