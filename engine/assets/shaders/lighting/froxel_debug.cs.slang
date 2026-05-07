/**
 * @name Irradiance Volume Froxels [1 thread per froxel]
 * ...
 */
import view;
import math;
import volume;
import visibility;
import voxel;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
Texture2D<float4> blue_noise;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
StructuredBuffer<VoxelObject> objects;
/* Froxel buffer. */
Texture3D<float3> froxel_buffer;
/* Linear sampler. */
SamplerState linear_sampler;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

static const uint STEP_COUNT = 32u;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Load pixel depth to correctly set maximum ray march depth */
    const uint64_t packed = asuint64(vbuffer[thread_id]);
    float max_t = iv::FROXEL_FAR_Z;
    if (!null_visibility(packed)) {
        const VisData vis = unpack_visibility(packed);
        const GeoData geo = unpack_geometry(vis, objects, ray);
        max_t = min(iv::FROXEL_FAR_Z, geo.depth);
    }
    const float step_t = max_t / (STEP_COUNT + 1u);
    const float jitter = blue_noise[thread_id % 512].r * step_t;

    /* Ray march the froxel volume */
    float3 luminance = 0.0f;
    float t = iv::FROXEL_NEAR_Z + jitter;
    for (int i = 0; i < STEP_COUNT; ++i) {
        /* Sample the froxel volume at each step */
        const float3 uvw = iv::froxel_uvw(ray.origin + ray.dir * t, render_view.world_to_clip, iv::FROXEL_NEAR_Z, iv::FROXEL_FAR_Z);
        luminance += froxel_buffer.SampleLevel(linear_sampler, uvw, 0);
        t += step_t; /* Step */
    }
    luminance /= STEP_COUNT;

    render_target[thread_id] = float4(luminance, 1.0f);
}
