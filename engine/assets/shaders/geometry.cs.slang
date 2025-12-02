import view;
import math;

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

/* Represents a renderable voxel object. */
struct VoxelObject {
    float4x4 world_to_local;
    float4x4 local_to_world;
    uint3 size;
};

inline void intersect_voxel_object(Ray ray, VoxelObject object, out float hit_t) {
    /* Transform the ray into the object local-space */
    const float3 local_origin = mul(object.world_to_local, float4(ray.origin, 1.0f)).xyz;
    const float3 local_rcp_dir = 1.0f / mul(object.world_to_local, float4(ray.dir, 0.0f)).xyz;

    /* Intersect the object AABB in local-space */
    const float3 half_extent = float3(object.size) * UNITS_PER_VOXEL * 0.5f;
    const float3 t_to_min = (-half_extent - local_origin) * local_rcp_dir;
    const float3 t_to_max = (half_extent - local_origin) * local_rcp_dir;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel object buffer. */
StructuredBuffer<VoxelObject> object_buffer;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    const VoxelObject object = object_buffer[0];
    float3 half_extent = float3(object.size) * UNITS_PER_VOXEL * 0.5f;

    float3 center = float3(object.local_to_world[0].w, object.local_to_world[1].w, object.local_to_world[2].w);
    float3 axis_x = float3(object.local_to_world[0].x, object.local_to_world[1].x, object.local_to_world[2].x);
    float3 axis_y = float3(object.local_to_world[0].y, object.local_to_world[1].y, object.local_to_world[2].y);
    float3 axis_z = float3(object.local_to_world[0].z, object.local_to_world[1].z, object.local_to_world[2].z);

    float3 extent = abs(axis_x * half_extent.x)
                   + abs(axis_y * half_extent.y)
                   + abs(axis_z * half_extent.z);

    float hit_aabb_t = 1e30f, hit_t = 1e30f;
    intersect_voxel_object(ray, object_buffer[0], hit_t);
    intersect_aabb(ray.origin, ray.rcp_dir, center - extent, center + extent, hit_aabb_t);
    if (hit_t < BIG_F32) {
        render_target[thread_id] = float4(1.0f - (hit_t * 0.25f).xxx, 1.0f);
    } else if (hit_aabb_t < BIG_F32) {
        render_target[thread_id] = float4(0.5f, 0.0f, 0.0f, 1.0f);
    } else {
        render_target[thread_id] = float4(0.05f, 0.05f, 0.05f, 1.0f);
    }
}
