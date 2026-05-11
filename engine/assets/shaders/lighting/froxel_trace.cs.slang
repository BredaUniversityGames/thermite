/**
 * @name Irradiance Volume Froxels [1 thread per froxel]
 * ...
 */
import view;
import math;
import volume;
import voxel;
import bvh2;

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> nodes;
StructuredBuffer<uint> indices;
StructuredBuffer<VoxelObject> objects;
/* Froxel buffer. */
Texture3D<float3> prev_froxel_buffer;
RWTexture3D<float3> froxel_buffer;
/* Linear sampler. */
SamplerState linear_sampler;

/* Compute a unique seed for a given thread and frame. */
[ForceInline] uint thread_seed(int3 thread_id, uint frame) {
    return thread_id.x + thread_id.y * iv::FROXEL_GRID_SIZE.x + thread_id.z * iv::FROXEL_GRID_SIZE.x * iv::FROXEL_GRID_SIZE.y + render_view.frame_index * iv::FROXEL_GRID_SIZE.x * iv::FROXEL_GRID_SIZE.y * iv::FROXEL_GRID_SIZE.z;
}

/* Anisotropic scattering phase function. */
[ForceInline] float henyey_greenstein(float cos_theta, float g) {
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * cos_theta;
    return (1.0f - g2) / (4.0f * PI_F32 * pow(denom, 1.5f));
}

struct Pc {
    float fog_anisotropy; /* default: 0.6 */
    float scatter_strength; /* default: 0.0025 */
};
[[vk::push_constant]] Pc p;

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= iv::FROXEL_GRID_SIZE)) return;

    /* Generate a unique seed for this thread */
    uint seed = thread_seed(thread_id, render_view.frame_index);
    /* Jitter the froxel ID using the unique seed */
    const float3 froxel_id = float3(thread_id) + rand_float3(seed);

    /* Trace occlusion for the sun */
    const float3 froxel_pos = iv::froxel_to_world(froxel_id, render_view.clip_to_world, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    const float t = occlusion_bvh2(nodes, indices, objects, custom_ray(froxel_pos, scene_view.sun_dir), BIG_F32);

    /* Calculate scattering */
    const float phase = henyey_greenstein(dot(normalize(froxel_pos - render_view.origin.xyz), scene_view.sun_dir), p.fog_anisotropy);
    const float3 luminance = select(t < BIG_F32, 0.0f, scene_view.sun_luminance) * phase * p.scatter_strength;

    /* Re-project history */
    const float3 froxel_center = iv::froxel_to_world(float3(thread_id) + 0.5f, render_view.clip_to_world, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    const float3 history_uvw = iv::froxel_uvw(froxel_center, render_view.prev_world_to_clip, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
    if (any(history_uvw < 0.0f) || any(history_uvw >= 1.0f)) {
        froxel_buffer[thread_id] = luminance;
        return;
    }
    const float3 history = prev_froxel_buffer.SampleLevel(linear_sampler, history_uvw, 0);

    /* Blend over-time with history */
    froxel_buffer[thread_id] = lerp(history.rgb, luminance, 0.05f);
}
