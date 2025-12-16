module voxel;

import math;
import view;
import svt64;
import material;
import bindless;

/* Represents a renderable voxel object. */
public struct VoxelObject {
    float4x4 world_to_local;
    float4x4 local_to_world;
    uint4 size_rcp_width; /* .xyz = voxel size ~ .w = reciprocal tree width */
    uint blas_handle;
    public uint voxels_handle;
    public uint palette_handle;
};

public [ForceInline] void intersect_voxel_object<let DEBUG : bool>(Ray ray, VoxelObject object, uint index, inout Hit hit) {
    /* Transform the ray into the local space of the object */
    const float3 world_origin = ray.origin;
    ray.origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    ray.dir = mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;
    ray.rcp_dir = 1.0f / ray.dir;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size_rcp_width.xyz) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_to_max = (half_extent - ray.origin) * ray.rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(min(t_max.x, t_max.y), t_max.z), hit.t);
    if (t_near > t_far) return;

    /* Calculate the ratio between the object size and its tree size */
    const float3 width_ratio = float3(object.size_rcp_width.xyz) * asfloat(object.size_rcp_width.w);

    /* Calculate the entry point for the tree traversal */
    const float3 entry_point = ray.origin + ray.dir * t_near;
    const float3 entry_uvw = (entry_point / half_extent) * 0.5f + 0.5f;
    ray.origin = (entry_uvw * width_ratio) + 1.0f; /* [1.0, 2.0) */

    /* Trace the SVT64 acceleration structure */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const Svt64Hit local_hit = trace_svt64<DEBUG>(nodes, ray);

    /* Check if we missed */
    if (DEBUG) hit.steps += local_hit.steps;
    if (local_hit.miss()) return;

    /* Calculate world-space hit point and distance */
    const float3 hit_uvw = (local_hit.point - 1.0f) / width_ratio;
    const float3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const float3 hit_point = mul(object.local_to_world, float4(hit_ndc * half_extent, 1.0f)).xyz;
    const float hit_dist = distance(world_origin, hit_point);
    if (hit_dist > hit.t) return;

    /* Overwrite the current hit with the closer one */
    hit.t = hit_dist;
    hit.voxel_index = local_hit.voxel_index;
    hit.object_index = index;
    hit.face_index = local_hit.face_index;
}

/* XXHash32 <https://jcgt.org/published/0009/03/02/> */
public inline uint xxhash32(const uint p) {
    const uint PRIME32_2 = 2246822519u, PRIME32_3 = 3266489917u;
    const uint PRIME32_4 = 668265263u, PRIME32_5 = 374761393u;
    uint h32 = p + PRIME32_5;
    h32 = PRIME32_4 * ((h32 << 17u) | (h32 >> (32u - 17u)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15u));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13u));
    return h32 ^ (h32 >> 16u);
}

public inline uint voxel_hash(uint voxel_index, uint object_index, uint face_index) {
    return xxhash32((voxel_index & 0x001FFFFFu)
        | ((object_index & 0x000000FFu) << 21u)
        | ((face_index & 0x00000007u) << 29u));
}
