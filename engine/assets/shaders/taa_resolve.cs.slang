import colorspace;
import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Point Sampler. */
SamplerState point_sampler;
/* Motion Vectors buffer. */
Texture2D<float2> mbuffer;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* History buffer. */
RWTexture2D<float4> hbuffer;
/* Render target. */
RWTexture2D<float4> render_target;

[[vk::push_constant]]
uint32_t taa_flag;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    float2 uv = (float2(thread_id) + 0.5f) / float2(render_view.resolution);

    float2 uv_last = uv + mbuffer.SampleLevel(point_sampler, uv, 0);

    float3 curr_luminance = lbuffer[thread_id];

    /* Linear Filtering Alternative :( */
     const float2 texel = uv_last * float2(render_view.resolution) - 0.5f;
     const float2 f = frac(texel);
     const int2 t = int2(floor(texel));
    
     const float3 tl = hbuffer[t].rgb;
     const float3 tr = hbuffer[t + int2(1, 0)].rgb;
     const float3 bl = hbuffer[t + int2(0, 1)].rgb;
     const float3 br = hbuffer[t + int2(1, 1)].rgb;
    
     float3 prev_luminance = lerp(lerp(tl, tr, f.x), lerp(bl, br, f.x), f.y);

    float3 output = float3(0.0f, 0.0f, 0.0f);
    if (taa_flag == 1) {
        output = lerp(prev_luminance, curr_luminance, 0.05f);

        hbuffer[thread_id] = float4(output, 1.0f);

        /* Neighborhood clamp to prevent ghosting */
        float3 neighbour_min = curr_luminance, neighbour_max = curr_luminance;
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                float3 s = lbuffer[thread_id + int2(x, y)];
                neighbour_min = min(neighbour_min, s);
                neighbour_max = max(neighbour_max, s);
            }
        }
        prev_luminance = clamp(prev_luminance, neighbour_min, neighbour_max);

        /* Reject off-screen */
        bool valid = all(uv_last >= 0.0f) && all(uv_last <= 1.0f);
        float blend = valid ? 0.95f : 0.0f;

        output = lerp(curr_luminance, prev_luminance, blend);
        hbuffer[thread_id] = float4(output, 1.0f);

    } else {
        output = curr_luminance;
    }

    /* Convert from ACEScg to r709 */
    output = cs::acescg_to_r709(output);

    /* Apply AGX tonemapping */
    output = cs::agx(output);
    output = cs::agx_look(output);
    output = cs::agx_eotf(output);

    /* Output device transform */
    output = cs::delinearize(output);

    render_target[thread_id] = float4(output, 1.0f);
}
