import view;
import voxel;
import bvh2;
import visibility;
import math;
import volume;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Probe output buffer. */
Texture2D<float3> probe_buffer;
/* Linear sampler. */
SamplerState linear_sampler;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[[vk::push_constant]] int cascade_index;

float trace_sphere(float3 ro, float3 rd, float3 center, float radius) {
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return -1.0;
    float t = -b - sqrt(disc);
    return t > 0.0 ? t : -1.0;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    if (any(thread_id >= render_view.resolution))
        return;

    const Ray ray = pinhole_ray(render_view, thread_id);

    static const float NEAR_Z = 0.1f;
    static const float FAR_Z = 320.0f;

    float closest_t = 1e30f;
    float3 hit_n = 0.0f;
    // float z = ((render_view.frame_index / 30) % 64);

    static const float3 SIZE = float3(160 / 4, 90 / 4, 64 / 4);

    // for (int z = 0; z < SIZE.z; ++z) {
    //     for (int y = 0; y < SIZE.y; ++y) {
    //         for (int x = 0; x < SIZE.x; ++x) {
    //             const float3 center = froxel_to_world(int3(x, y, z), render_view.clip_to_view, 0.1f, 320.0f);

    //             // const float3 center = float3(x, y, z);
    //             const float t = trace_sphere(ray.origin, ray.dir, center, 0.1f);

    //             if (t > 0.0f && t < closest_t) {
    //                 // hit_n = normalize((ray.origin + t * ray.dir) - center);
    //                 hit_n = froxel_uvw(center, inverse(render_view.clip_to_view), 0.1f, 320.0f);
    //                 closest_t = t;
    //             }
    //         }
    //     }
    // }

    if (closest_t < 1e30f) {
        render_target[thread_id] = float4(hit_n, 1.0f);
    } else {
        // render_target[thread_id] = float4(0.1f, 0.1f, 0.1f, 1.0f);
    }

    // /* Grid constants */
    // const float3 cascade_center = 0.0f;
    // const float probe_extent = float(CASCADE_EXTENT) / CASCADE_WIDTH;
    // const float3 grid_min = cascade_center - CASCADE_HALF_EXTENT;
    // const float3 grid_max = cascade_center + CASCADE_HALF_EXTENT;

    // /* Early out if the ray misses the cascade entirely */
    // const float3 inv_dir = rcp(ray.dir);
    // const float3 t0 = (grid_min - ray.origin) * inv_dir;
    // const float3 t1 = (grid_max - ray.origin) * inv_dir;
    // const float3 t_min = min(t0, t1);
    // const float3 t_max = max(t0, t1);
    // const float t_enter = max(max(t_min.x, t_min.y), max(t_min.z, 0.0f));
    // const float t_exit = min(min(t_max.x, t_max.y), t_max.z);
    // if (t_enter > t_exit) return;

    // /* DDA setup */
    // const float3 entry = ray.origin + ray.dir * (t_enter + 0.01f);
    // const float3 grid_local = (entry - grid_min) / probe_extent;
    // int3 cell = clamp(int3(floor(grid_local)), int3(0, 0, 0), CASCADE_WIDTH - 1);
    // const int3 step_dir = select(ray.dir >= 0.0f, 1, -1);
    // const float3 t_delta = abs(probe_extent * inv_dir);
    // const float3 next_bound = grid_min + (float3(cell) + float3(step_dir) * 0.5f + 0.5f) * probe_extent;
    // float3 t_max3 = (next_bound - ray.origin) * inv_dir;
    // const int3 exit_cell = select(step_dir > 0, CASCADE_WIDTH, -1);

    // /* DDA traversal state */
    // float closest_t = 1e30f;
    // int3 hit_probe = int3(-1, -1, -1);
    // float3 hit_normal = float3(0.0f, 0.0f, 0.0f);

    // /* Traverse the cascade using DDA */
    // for (int i = 0; i < 48; i++) {
    //     /* For each cell trace against a sphere representing the probe */
    //     const float3 center = grid_min + (float3(cell) + 0.5f) * probe_extent;
    //     const float t = trace_sphere(ray.origin, ray.dir, center, 0.2f);

    //     /* Check if we hit the probe sphere */
    //     if (t > 0.0f && t < closest_t) {
    //         closest_t = t;
    //         hit_probe = cell;
    //         hit_normal = normalize((ray.origin + t * ray.dir) - center);
    //         break;
    //     }

    //     /* Advance along axis with smallest t_max */
    //     if (t_max3.x < t_max3.y && t_max3.x < t_max3.z) {
    //         cell.x += step_dir.x; t_max3.x += t_delta.x;
    //         if (cell.x == exit_cell.x) break;
    //     } else if (t_max3.y < t_max3.z) {
    //         cell.y += step_dir.y; t_max3.y += t_delta.y;
    //         if (cell.y == exit_cell.y) break;
    //     } else {
    //         cell.z += step_dir.z; t_max3.z += t_delta.z;
    //         if (cell.z == exit_cell.z) break;
    //     }
    // }

    // /* Missed */
    // if (closest_t == 1e30) return;

    // /* Calculate the UV coordinates to sample on the probe atlas */
    // const float2 ray_id = saturate(oct_encode(hit_normal)) * PROBE_WIDTH;
    // const float2 probe_id = sample_texel(hit_probe, cascade_index);
    // const float2 uv = (probe_id + ray_id) / ATLAS_SIZE;

    // /* Sample the probe atlas and display the color */
    // render_target[thread_id] = float4(probe_buffer.SampleLevel(linear_sampler, uv, 0) / 16.0f, 1.0f);
}
