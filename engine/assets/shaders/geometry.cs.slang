static const float BIG_F32 = 1e30;

/* Ray AABB intersection function. *(returns BIG_F32 if the ray misses the AABB)* */
inline void intersect_aabb(float3 ro, float3 rrd, float3 bmin, float3 bmax, out float hit_t) {
    const float3 t_to_min = (bmin - ro) * rrd;
    const float3 t_to_max = (bmax - ro) * rrd;
    const float3 t_min = min(t_to_min, t_to_max);
    const float3 t_max = max(t_to_min, t_to_max);
    const float t_near = max(max(max(t_min.x, t_min.y), t_min.z), 0.0);
    const float t_far = min(min(t_max.x, t_max.y), t_max.z);
    hit_t = t_near > t_far ? BIG_F32 : t_near;
}

struct VoxelObject {
    float4x4 world_to_local;
    uint3 size;
};

/* Bindings */ 
StructuredBuffer<VoxelObject> object_buffer;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {

}
