import lights;
import view;
import debug;
import math;

import lightgrid;

/* Active render view. (Input)*/
ConstantBuffer<RenderView> render_view;
/* Depth buffer. (Input)*/
Texture2D<float> dbuffer;
/* Light grid buffer. (Input) */
StructuredBuffer<uint32_t> light_grid;
/* Lights buffer. (Input) */
StructuredBuffer<Light> lights;
/* Render target. (Input/Output)*/
RWTexture2D<float4> render_target;

[[vk::push_constant]]
float3 cam_center;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Sample depth buffer */
    const float depth = dbuffer[thread_id];
    if (depth == 0.0f || depth >= BIG_F32) { /* sky — skip or write black */
        return;
    }

    /* Reconstruct world position from depth */
    uint2 resolution;
    render_target.GetDimensions(resolution.x, resolution.y);
    const float2 uv = (float2(thread_id) + 0.5f) / float2(resolution);
    const float2 ndc = uv * 2.0f - 1.0f;

    const float4 clip_pos = float4(ndc, depth, 1.0f);
    float4 world_pos = mul(render_view.clip_to_world, clip_pos);
    world_pos /= world_pos.w;

    /* Look up cascade */
    const CascadeIndex index = index_cascades(world_pos.xyz - cam_center);

    if (index.cascade == 0xFFFFFFFFu || index.cascade >= 3u) {
        return;
    }

    const float3 cascade_colors[3] = {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f),
    };
    const float3 cascade_color = cascade_colors[min(index.cascade, 2u)];

    const uint32_t cell_index = (index.cascade * CASCADES_RESOLUTION3) + (index.z * CASCADES_RESOLUTION2) + (index.y * CASCADES_RESOLUTION) + index.x;
    const uint32_t count = light_grid[cell_index * LIGHTS_PER_CELL];
    const float3 heat = heat_color((float)count / 4.0f);

    /* Blend debug color over the scene */
    const float3 scene = render_target[thread_id].rgb;
    const float3 debug_color = lerp(cascade_color * 0.25f, cascade_color, heat);
    const float3 color = lerp(scene, debug_color, 0.6f);

    render_target[thread_id] = float4(color, 1.0f);
}
