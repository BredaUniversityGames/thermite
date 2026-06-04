import bindless;
import ui_text;

/* GPU resources */
[[vk::binding(1, 0)]]
StructuredBuffer<GlyphInstance> glyphs;
[[vk::binding(2, 0)]]
SamplerState text_sampler;

/* MSDF median */
[ForceInline]
float median(const float3 msdf) {
    return max(min(msdf.r, msdf.g), min(max(msdf.r, msdf.g), msdf.b));
}

float screen_px_range(Texture2D<float4> atlas, float2 uv, float px_range) {
    uint aw, ah;
    atlas.GetDimensions(aw, ah);
    const float2 unit_range = float2(px_range, px_range) / float2(aw, ah);
    const float2 screen_tex_size = float2(1.0f, 1.0f) / fwidth(uv);
    return max(0.5f * dot(unit_range, screen_tex_size), 1.0f);
}

[shader("pixel")]
float4 main(TextVertexOutput input) : SV_Target {
    /* Get glyph instance data */
    const GlyphInstance glyph = glyphs[input.instance_id];

    /* Get the font atlas texture using bindless */
    const uint atlas_idx = glyph.atlas_index();
    if (atlas_idx == 0u) {
        /* No texture, return solid color (shouldn't happen with proper fonts) */
        return glyph.color;
    }
    const Texture2D<float4> atlas = bindless::texture_sampled<Texture2D<float4>>(atlas_idx);

    /* Read and interpret multi-channel SDF */
    const float3 msdf = atlas.Sample(text_sampler, input.uvs).rgb;
    const float sd = median(msdf);

    const float spr = screen_px_range(atlas, input.uvs, 16.0f /* font.px_range */);
    const float dist_px = (sd - 0.5f) * spr;         // signed, + = inside glyph
    const float opacity = saturate(dist_px + 0.5f);  // 1px AA, exact

    float3 rgb = float3(0.0f);
    float alpha = 0.0f;
    if (glyph.glow_strength() > 0.0f) {
        const float radius = max(glyph.glow_radius(), 1e-3f);
        const float range_px = 0.5f * spr;
        const float out_px = max(0.0f, -dist_px);

        /* Normalized 0..1 across the SDF band, soft falloff, zero at boundary by construction */
        float t = saturate(out_px / range_px);
        float glow_a = pow(1.0f - t, 2.0f);  // raise exponent = tighter, lower = softer/wider

        const float size_boost = saturate(radius / range_px - 1.0f);
        glow_a *= 1.0f + size_boost * glyph.glow_boost();

        /* Additive: glow is light, not an occluding band */
        const float3 core_pm = glyph.color.rgb * opacity;
        const float3 glow_add = glyph.glow_color.rgb * glyph.glow_strength() * glow_a * (1.0f - opacity);

        rgb = saturate((core_pm + glow_add)) * glyph.color.a;
        alpha = saturate(opacity + glow_a * glyph.glow_strength() * (1.0f - opacity)) * glyph.color.a;
    } else {
        rgb = glyph.color.rgb * opacity * glyph.color.a;
        alpha = opacity * glyph.color.a;
    }

    return float4(rgb, alpha);
}
