import view;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Down Sampler */
SamplerState down_sampler;
/* Mip imaage views. */
RWTexture2D<float3> curr_mip;
Texture2D<float3> prev_mip;

[[vk::push_constant]]
 uint32_t curr_mip_level;

static const float2 BloomCoords[13] = float2[13](
    float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0), float2(1.0, -1.0),

    float2(-2.0, 2.0), float2(0.0, 2.0), float2(2.0, 2.0), float2(-2.0, 0.0), float2(0.0, 0.0), float2(2.0, 0.0), float2(-2.0, -2.0), float2(0.0, -2.0), float2(2.0, -2.0)
);

float MaxBrightness(float3 Color)
{
    return max(max(Color.r, Color.g), Color.b);
}

float3 AverageBlock(float3 Sample1, float3 Sample2, float3 Sample3, float3 Sample4) {
    float4 SampleBrightness = float4(MaxBrightness(Sample1), MaxBrightness(Sample2), MaxBrightness(Sample3), MaxBrightness(Sample4));

    float4 Weights = 1.0f / (SampleBrightness + 1.0f);
    float WeightSum = Weights.x + Weights.y + Weights.z + Weights.w;

    float3 Result = (Sample1 * Weights.x + Sample2 * Weights.y + Sample3 * Weights.z + Sample4 * Weights.w) * (1.0f / WeightSum);

    return Result;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(uint2 thread_id: SV_DispatchThreadID) {
    /* Current mip dimensions. */
    float2 dst_res = float2(render_view.resolution >> curr_mip_level);
    /* Source (previous) mip dimensions. */
    float2 src_res = float2(render_view.resolution >> (curr_mip_level - 1));

    /* Early out for threads beyond the current mip's extent. */
    if (any(thread_id >= uint2(dst_res)))
        return;

    float2 uv = (float2(thread_id) + 0.5f) / dst_res;
    float2 src_texel_size = 1.0f / src_res;

    float3 Samples[13];
    for (int i = 0; i < 13; i++) {
        float2 sample_uv = uv + BloomCoords[i] * src_texel_size;
        Samples[i] = prev_mip.SampleLevel(down_sampler, sample_uv, 0.0f).rgb;
    }

    float3 Center = AverageBlock(Samples[0], Samples[1], Samples[2], Samples[3]);
    float3 TopLeft = AverageBlock(Samples[4], Samples[5], Samples[8], Samples[7]);
    float3 TopRight = AverageBlock(Samples[5], Samples[6], Samples[9], Samples[8]);
    float3 BottomLeft = AverageBlock(Samples[7], Samples[8], Samples[11], Samples[10]);
    float3 BottomRight = AverageBlock(Samples[8], Samples[9], Samples[12], Samples[11]);

    curr_mip[thread_id] = 0.5f * Center + 0.125f * (TopLeft + TopRight + BottomLeft + BottomRight);
}
