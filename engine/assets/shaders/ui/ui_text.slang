module ui_text;

/*
 * Shared structures for text rendering using SDF (Signed Distance Field) fonts.
 * 
 * The SDF approach allows:
 *   - Scalable text at any size without loss of quality
 *   - Smooth anti-aliased edges
 *   - Outlines, glows, and shadows with minimal GPU cost
 *   - Text effects like boldness adjustment
 */

/* Vertex input - same as UI images, simple quad */
public struct TextVertexInput {
    public float3 pos;
    public float2 uvs;
};

/* Vertex output to pixel shader */
public struct TextVertexOutput {
    public float4 pos : SV_Position;
    public float2 uvs : TEX_COORD0;
    public int instance_id;
};

/* 
 * Data for a single glyph instance.
 * Packed for efficient GPU memory usage (aligned to 16 bytes).
 */
public struct GlyphInstance {
    /* Position and size */
    public float4 pos_size;           /* .xy = screen position, .zw = glyph size */
    
    /* UV coordinates in the font atlas */
    public float4 uv_rect;            /* .xy = min UV, .zw = max UV */
    
    /* Text color (RGBA) */
    public float4 color;
    
    /* Outline color (RGBA) */
    public float4 outline_color;
    
    /* SDF parameters */
    public float4 params;             /* .x = atlas_index, .y = outline_width, .z = softness, .w = boldness */
    
    /* Shadow parameters */
    public float4 shadow_params;      /* .xy = shadow_offset, .z = shadow_softness, .w = unused */
    
    /* Shadow color (RGBA) */
    public float4 shadow_color;
    
    /* Glow parameters */
    public float4 glow_params;        /* .x = glow_width, .yzw = unused */
    
    /* Glow color (RGBA) */
    public float4 glow_color;

    /* Accessor functions for cleaner shader code */
    public float2 pos() { return pos_size.xy; }
    public float2 size() { return pos_size.zw; }
    public float2 uv_min() { return uv_rect.xy; }
    public float2 uv_max() { return uv_rect.zw; }
    public uint atlas_index() { return asuint(params.x); }
    public float outline_width() { return params.y; }
    public float softness() { return params.z; }
    public float boldness() { return params.w; }
    public float2 shadow_offset() { return shadow_params.xy; }
    public float shadow_softness() { return shadow_params.z; }
    public float glow_width() { return glow_params.x; }
};

/*
 * SDF helper functions for text rendering effects.
 */

/* 
 * Calculate alpha from SDF distance value.
 * 
 * @param distance The signed distance (0.5 = edge, >0.5 = inside, <0.5 = outside)
 * @param softness Edge softness for anti-aliasing
 * @param boldness Adjustment to make text thicker (positive) or thinner (negative)
 * @return Alpha value [0, 1]
 */
public float sdf_alpha(float distance, float softness, float boldness) {
    /* Adjust the edge threshold based on boldness */
    float edge = 0.5 - boldness * 0.25;
    
    /* Calculate smoothstep for anti-aliasing */
    float soft = max(softness, 0.001); /* Prevent division by zero */
    return smoothstep(edge - soft, edge + soft, distance);
}

/*
 * Calculate alpha for an outline effect.
 * 
 * @param distance The signed distance value
 * @param outline_width Width of the outline
 * @param softness Edge softness
 * @return Alpha value [0, 1] for the outline region
 */
public float sdf_outline_alpha(float distance, float outline_width, float softness) {
    float soft = max(softness, 0.001);
    float outer_edge = 0.5 - outline_width;
    float inner_edge = 0.5;
    
    /* Outline is visible between outer and inner edge */
    float outer = smoothstep(outer_edge - soft, outer_edge + soft, distance);
    float inner = smoothstep(inner_edge - soft, inner_edge + soft, distance);
    
    return outer - inner;
}

/*
 * Calculate alpha for a glow effect.
 * 
 * @param distance The signed distance value
 * @param glow_width Width of the glow
 * @param softness Edge softness
 * @return Alpha value [0, 1] for the glow region
 */
public float sdf_glow_alpha(float distance, float glow_width, float softness) {
    float soft = max(softness, 0.001);
    float glow_edge = 0.5 - glow_width;
    
    /* Glow fades out from the edge */
    return smoothstep(glow_edge - soft, 0.5, distance);
}