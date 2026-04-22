import bindless;
import particles;
import math;
import view;
import colorspace;
import volume;

ConstantBuffer<RenderView> gpu_view;

[[vk::binding(3, 0)]]
Texture2D<float4> blue_noise1d;

[[vk::binding(4, 0)]]
SamplerState point_sampler;

/* Probe output buffer. */
[[vk::binding(5, 0)]]
Texture2D<float3> probe_buffer;
[[vk::binding(6, 0)]]
SamplerState linear_sampler;

struct FragmentOutput {
    float4 color : SV_Target0;
    float2 motion : SV_Target1;
};

[shader("fragment")]
FragmentOutput main(BillboardVertexOutput input, uint32_t instance_id: SV_InstanceID) {
    Texture2D<float4> tex = bindless::texture_sampled<Texture2D<float4>>(input.tex_index);

    const float frame = floor(input.current_frame);
    const float2 anim_uv = float2((frame + input.uv.x) / input.flipbook_frames, input.uv.y);

    const float4 tex_sample = tex.Sample(point_sampler, anim_uv);

    const float2 screen_pos = input.pos.xy;
    const bool partial_transparent = tex_sample.a > 0.0f && tex_sample.a < 1.0f;
    const bool should_dither = input.opacity <= 0.99f || partial_transparent;

    const float dither = should_dither ? r1(blue_noise1d.Sample(point_sampler, screen_pos / (512.0f * input.dither_scale)).r, gpu_view.frame_index % 1024u) : 0.0f;

    // dither when alpha is above 0.0f but below 1.0f
    if (tex_sample.a * input.opacity <= dither)
        discard;

    FragmentOutput output;

    /* Sample the irradiance volume */
    const float3 world_pos = input.world_pos;
    const float2 probe_id = iv::probe_uv(world_pos);
    const float2 ray_id = saturate(oct_encode(normalize(gpu_view.origin.xyz - world_pos))) * iv::PROBE_WIDTH;
    const float2 uv = (probe_id + ray_id) / iv::ATLAS_SIZE;

    output.color = float4(cs::r709_to_acescg(cs::linearize(tex_sample.rgb * probe_buffer.SampleLevel(linear_sampler, uv, 0))), 1.0f);
    output.motion = input.motion_vec;

    return output;
}