import bindless;
import particles;

[[vk::binding(3, 0)]]
SamplerState point_sampler;

[shader("fragment")]
float4 main(BillboardVertexOutput input) : SV_Target0 {
    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    const float frame = floor(input.current_frame);
    const float2 anim_uv = float2((frame + input.uv.x) / input.flipbook_frames, input.uv.y);

    const float4 tex_sample = tex.Sample(point_sampler, anim_uv);

    return float4(tex_sample.rgb, input.opacity);
}