module voxel;

import math;
import view;
import svt64;
import bindless;

/* Represents a renderable voxel object. */
public struct VoxelObject {
    float4x4 world_to_local;
    float4x4 local_to_world;
    uint size_x;
    uint size_y;
    uint size_z;
    uint blas_handle;
    uint voxels_handle;
};

public inline void intersect_voxel_object(Ray ray, VoxelObject object, out float hit_t) {
    const StructuredBuffer<Svt64Node> blas = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(6);
    const StructuredBuffer<uint8_t> voxels = bindless::storage_buffer<StructuredBuffer<uint8_t>>(7);

    ray.origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    ray.dir = mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;
    ray.rcp_dir = 1.0f / ray.dir;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size_x, object.size_y, object.size_z) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    if (t_near > t_far) {
        hit_t = BIG_F32;
        return;
    }

    /* Move to the point of entry */
    ray.origin = ray.origin + ray.dir * t_near;

    /* Move the origin into [1, 2) space for 64 tree traversal */
    ray.origin = ray.origin / half_extent * 0.5f + 1.5f; // (ray.origin + half_extent) / (half_extent * 2.0f) + 1.0f;

    hit_t = t_near + trace_svt64(blas, voxels, ray);

    /* Transform the ray into the object local-space */
    // const float3 offset = float3(object.size_x, object.size_y, object.size_z);
    // const float3 local_origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    // const float3 local_rcp_dir = 1.0f / mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;

    /* Intersect the object AABB in local-space */
    // const float3 half_extent = float3(object.size_x, object.size_y, object.size_z) * UNITS_PER_VOXEL * 0.5f;
    // const float3 t_to_min = (-half_extent - local_origin) * local_rcp_dir;
    // const float3 t_to_max = (half_extent - local_origin) * local_rcp_dir;
    // const float3 t_min = min(t_to_min, t_to_max);
    // const float3 t_max = max(t_to_min, t_to_max);
    // const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    // const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    // hit_t = t_near > t_far ? BIG_F32 : t_near;
}
