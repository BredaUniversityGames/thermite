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

// --- Ray-sphere intersection, returns t or -1 ---
float RaySphere(float3 ro, float3 rd, float3 center, float radius) {
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

    /* Grid constants */
    static const int3 GRID_SIZE = int3(16, 16, 16);
    static const float SPACING = 2.0f;
    static const float3 GRID_ORIGIN = -16.0f;
    static const float3 GRID_EXTENT = 32.0f;
    static const float3 GRID_HALF_EXTENT = GRID_EXTENT * 0.5f;
    static const float RADIUS = 0.2f;

    float3 grid_min = GRID_ORIGIN; // - GRID_HALF_EXTENT;
    float3 grid_max = GRID_ORIGIN + GRID_EXTENT;

    /* Ray-AABB slab test */
    float3 inv_dir = rcp(ray.dir);
    float3 t0 = (grid_min - ray.origin) * inv_dir;
    float3 t1 = (grid_max - ray.origin) * inv_dir;
    float3 t_min = min(t0, t1);
    float3 t_max = max(t0, t1);
    float t_enter = max(max(t_min.x, t_min.y), max(t_min.z, 0.0f));
    float t_exit = min(min(t_max.x, t_max.y), t_max.z);

    if (t_enter > t_exit) {
        // render_target[thread_id] = float4(0.1f, 0.1f, 0.1f, 1.0f);
        return;
    }

    /* DDA setup */
    float3 entry = ray.origin + ray.dir * (t_enter + 0.01f);
    float3 grid_local = (entry - GRID_ORIGIN) / SPACING;
    int3 cell = clamp(int3(floor(grid_local)), int3(0, 0, 0), GRID_SIZE - 1);
    int3 step_dir = int3(ray.dir >= 0.0f ? 1 : -1);
    float3 t_delta = abs(SPACING * inv_dir);

    float3 next_bound = GRID_ORIGIN
                      + (float3(cell) + float3(step_dir) * 0.5f + 0.5f) * SPACING;
    float3 t_max3 = (next_bound - ray.origin) * inv_dir;

    int3 exit_cell = int3(
        step_dir.x > 0 ? GRID_SIZE.x : -1,
        step_dir.y > 0 ? GRID_SIZE.y : -1,
        step_dir.z > 0 ? GRID_SIZE.z : -1
    );

    /* DDA march */
    float closest_t = 1e30;
    int3 hit_probe = int3(-1, -1, -1);
    float3 hit_normal = float3(0, 0, 0);

    [loop]
    for (int i = 0; i < 48; i++) {  // 3*16 max diagonal steps
        float3 center = GRID_ORIGIN + (float3(cell) + 0.5f) * SPACING;
        float t = RaySphere(ray.origin, ray.dir, center, RADIUS);

        if (t > 0.0f && t < closest_t) {
            closest_t = t;
            hit_probe = cell;
            hit_normal = normalize((ray.origin + t * ray.dir) - center);
            break;  // first hit is closest — sphere fits in cell
        }

        /* Advance along axis with smallest t_max */
        if (t_max3.x < t_max3.y && t_max3.x < t_max3.z) {
            cell.x += step_dir.x; t_max3.x += t_delta.x;
            if (cell.x == exit_cell.x) break;
        } else if (t_max3.y < t_max3.z) {
            cell.y += step_dir.y; t_max3.y += t_delta.y;
            if (cell.y == exit_cell.y) break;
        } else {
            cell.z += step_dir.z; t_max3.z += t_delta.z;
            if (cell.z == exit_cell.z) break;
        }
    }

    if (closest_t == 1e30) {
        // render_target[thread_id] = float4(0.1f, 0.1f, 0.1f, 1.0f);
        return;
    }

    const float2 ray_id = saturate(oct_encode(hit_normal)) * 8.0f;
    const float2 probe_id = sample_texel(hit_probe, cascade_index);
    const float2 uv = (probe_id + ray_id) / ATLAS_SIZE;

    // const float3 uvw = float3(
    //     (float(hit_probe.x) * 8.0f + uv.x * 8.0f) / (16.0f * 8.0f),
    //     (float(hit_probe.y) * 8.0f + uv.y * 8.0f) / (16.0f * 8.0f),
    //     float(hit_probe.z) / (16.0f * 6.0f)
    // );

    // render_target[thread_id] = float4(uvw, 1.0f);
    // render_target[thread_id] = float4(hit_normal * 0.5f + 0.5f, 1.0f);
    render_target[thread_id] = float4(probe_buffer.SampleLevel(linear_sampler, uv, 0) / 16.0f, 1.0f);
}
