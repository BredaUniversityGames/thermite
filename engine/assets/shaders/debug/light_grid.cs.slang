import lights;
import view;
import debug;
import math;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Depth buffer. */
Texture2D<float> dbuffer;
/* Light grid buffer. (Input) */
StructuredBuffer<uint32_t> light_grid;
/* Lights buffer. (Input) */
StructuredBuffer<Light> lights;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

struct CascadeIndex {
    uint32_t cascade = 0u;
    uint32_t x = 0u;
    uint32_t y = 0u;
    uint32_t z = 0u;
};

/* Index light cascades using view-space point `p`. */
inline CascadeIndex index_cascades(const float3 p) {
    /* Calculate the cascade index */
    const float max_axis = max(max(abs(p.x), abs(p.y)), abs(p.z));
    const uint32_t cascade_index = (uint32_t)max(0.0f, ceil(log2(max_axis) - 4.0f));
    if (cascade_index >= 8u)
        return { 0xFFFFFFFFu, 0u, 0u, 0u }; /* Out of bounds */

    /* Find which cell in the cascade the local point falls into */
    const uint32_t cascade_cell_extent = 1u << (cascade_index + 1u);
    const float3 cascade_point = floor(p / (float)cascade_cell_extent) + 8.0f;
    return { cascade_index, (uint32_t)cascade_point.x, (uint32_t)cascade_point.y, (uint32_t)cascade_point.z };
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Sample depth buffer */
    float depth = dbuffer[thread_id];
    if (depth == 0.0f || depth >= BIG_F32) { /* sky — skip or write black */
        render_target[thread_id] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    /* Reconstruct world position from depth */
    uint2 resolution;
    render_target.GetDimensions(resolution.x, resolution.y);
    float2 uv = (float2(thread_id) + 0.5f) / float2(resolution);
    float2 ndc = uv * 2.0f - 1.0f;
    //ndc.y = -ndc.y;

    float4 clip_pos = float4(ndc, depth, 1.0f);
    float4 world_pos = mul(render_view.clip_to_world, clip_pos);
    world_pos /= world_pos.w;

    /* Look up cascade */
    CascadeIndex index = index_cascades(world_pos.xyz);

    if (index.cascade == 0xFFFFFFFFu) {
        render_target[thread_id] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    const float3 cascade_colors[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f),
    };
    float3 cascade_color = cascade_colors[min(index.cascade, 2u)];

    const uint32_t cell_index = index.cascade * (16u * 16u * 16u)
                              + index.z * (16u * 16u)
                              + index.y * 16u
                              + index.x;
    const uint32_t count = light_grid[cell_index * 64u];
    float intensity = saturate((float)count / 1.0f);

    /* Blend debug color over the scene */
    float3 scene_color = render_target[thread_id].rgb;
    float3 debug_color = lerp(cascade_color * 0.15f, cascade_color, intensity);
    float3 color = lerp(scene_color, debug_color, 0.6f);

    render_target[thread_id] = float4(heat_color(count), 1.0f);
    //render_target[thread_id] = float4(cascade_color, 1.0f);
    //render_target[thread_id] = float4(frac(world_pos.xyz / 50.0f), 1.0f);
}