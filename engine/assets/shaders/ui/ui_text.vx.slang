import view;
import ui_text;

/* GPU resources */
ConstantBuffer<RenderView> gpu_view;
StructuredBuffer<GlyphInstance> glyphs;

[shader("vertex")]
TextVertexOutput main(TextVertexInput input, int instance_id: SV_InstanceID) {
    /* Get the glyph instance data */
    const GlyphInstance glyph = glyphs[instance_id];
    
    /* Calculate screen position */
    /* input.pos.xy is [0,1] representing the quad corners */
    const float2 screen_pos = glyph.pos() + input.pos.xy * glyph.size();
    
    /* Convert to normalized device coordinates (same as ui_image shader) */
    const float2 ndc = screen_pos / gpu_view.resolution;
    
    /* Interpolate UVs from glyph's UV rect */
    const float2 uvs = lerp(glyph.uv_min(), glyph.uv_max(), input.uvs);
    
    /* Output */
    TextVertexOutput output;
    output.pos = float4(ndc * 2.0f - 1.0f, 0.0f, 1.0f);
    output.uvs = uvs;
    output.instance_id = instance_id;
    
    return output;
}