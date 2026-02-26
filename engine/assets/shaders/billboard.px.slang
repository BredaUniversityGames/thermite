import bindless;
import particles;

[[vk::binding(3, 0)]]
SamplerState point_sampler;

float bayer8x8(float2 screen_pos, float scale) {
    uint2 p = uint2(screen_pos / scale) % 8;
    const float bayer[64] = {
         0,  32, 8,  40, 2,  34, 10, 42,
        48, 16, 56, 24, 50, 18, 58, 26,
        12, 44, 4,  36, 14, 46, 6,  38,
        60, 28, 52, 20, 62, 30, 54, 22,
         3,  35, 11, 43, 1,  33, 9,  41,
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47, 7,  39, 13, 45, 5,  37,
        63, 31, 55, 23, 61, 29, 53, 21
    };
    return bayer[p.y * 8 + p.x] / 64.0f;
}

[shader("fragment")]
float4 main(BillboardVertexOutput input) : SV_Target0 {
    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    const float frame = floor(input.current_frame);
    const float2 anim_uv = float2((frame + input.uv.x) / input.flipbook_frames, input.uv.y);

    const float4 tex_sample = tex.Sample(point_sampler, anim_uv);

    float2 screen_pos = input.pos.xy;
    float dither = bayer8x8(input.pos.xy, 0.001f);

    if (tex_sample.a == 0.0f || input.opacity < dither)
        discard;

    return float4(tex_sample.rgb, 1.0f);
}