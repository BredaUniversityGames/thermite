/*
 * Text Pixel Shader
 * 
 * Renders SDF (Signed Distance Field) text with support for:
 *   - Anti-aliased edges
 *   - Outlines
 *   - Shadows
 *   - Glow effects
 *   - Boldness adjustment
 */

import bindless;
import ui_text;

/* GPU resources */
[[vk::binding(1, 0)]]
StructuredBuffer<GlyphInstance> glyphs;
[[vk::binding(2, 0)]]
SamplerState text_sampler;

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
    
    /* Sample the SDF value from the font atlas */
    /* The atlas stores distance in the red/alpha channel */
    /* Values > 0.5 are inside the glyph, < 0.5 are outside */
    float dist = atlas.SampleLevel(text_sampler, input.uvs, 0).r;
    
    /* Get SDF parameters */
    float softness = glyph.softness();
    float boldness = glyph.boldness();
    float outline_width = glyph.outline_width();
    float glow_width = glyph.glow_width();
    
    /* Auto-calculate softness for anti-aliasing based on screen-space derivatives */
    /* The idea: compute how much the UV changes per pixel to determine edge width */
    float2 dpdx = ddx(input.uvs);
    float2 dpdy = ddy(input.uvs);
    float uv_per_pixel = length(float2(length(dpdx), length(dpdy)));
    
    /* Scale factor to convert UV-space distance to SDF-space smoothing */
    /* Larger values = softer edges, smaller = sharper (more aliased) */
    float auto_softness = uv_per_pixel * 0.7;
    
    /* Use auto-softness if no explicit softness specified, otherwise blend them */
    if (softness <= 0.0) {
        softness = auto_softness;
    } else {
        /* Ensure minimum softness for anti-aliasing even when explicit value is set */
        softness = max(softness, auto_softness * 0.5);
    }
    
    /* Start with transparent */
    float4 result = float4(0.0, 0.0, 0.0, 0.0);
    
    /* Layer 1: Shadow (rendered first, behind everything) */
    if (glyph.shadow_color.a > 0.0 && (glyph.shadow_offset().x != 0.0 || glyph.shadow_offset().y != 0.0)) {
        /* Sample shadow at offset position */
        /* Note: We'd need to pass proper offset UVs, for now use same position with softened edge */
        float shadow_soft = softness + glyph.shadow_softness() * 0.1;
        float shadow_alpha = sdf_alpha(dist, shadow_soft, boldness - 0.1);
        float4 shadow = glyph.shadow_color;
        shadow.a *= shadow_alpha;
        result = shadow;
    }
    
    /* Layer 2: Glow (behind outline and fill) */
    if (glow_width > 0.0 && glyph.glow_color.a > 0.0) {
        float glow_alpha = sdf_glow_alpha(dist, glow_width, softness);
        float4 glow = glyph.glow_color;
        glow.a *= glow_alpha;
        
        /* Blend glow over shadow */
        result = lerp(result, glow, glow.a);
    }
    
    /* Layer 3: Outline (between glow and fill) */
    if (outline_width > 0.0 && glyph.outline_color.a > 0.0) {
        /* Outline is the region between (0.5 - outline_width) and 0.5 */
        float outer_edge = 0.5 - outline_width;
        float inner_edge = 0.5 - boldness * 0.25;
        
        /* Smooth outer edge of outline */
        float outline_outer = smoothstep(outer_edge - softness, outer_edge + softness, dist);
        
        /* Full outline color */
        float4 outline = glyph.outline_color;
        outline.a *= outline_outer;
        
        /* Blend outline over glow */
        result = lerp(result, outline, outline.a);
    }
    
    /* Layer 4: Fill (the actual text, on top) */
    float fill_alpha = sdf_alpha(dist, softness, boldness);
    float4 fill = glyph.color;
    fill.a *= fill_alpha;
    
    /* Blend fill over outline/glow/shadow */
    result = lerp(result, fill, fill.a);
    
    /* Premultiplied alpha output */
    return result;
}