import view;

ConstantBuffer<RenderView> render_view;

/* Vertex shader input. */
struct VertexIn {
    float3 point_a : POSITION0;
    float3 point_b : POSITION1;
};

/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
    float4 color : COLOR;
};

/* 2D vertices for a quad. */
static const float2 quad[6] = {
    float2(-0.5f, 0.0f),
    float2(-0.5f, 1.0f),
    float2( 0.5f, 1.0f),
    float2(-0.5f, 0.0f),
    float2( 0.5f, 1.0f),
    float2( 0.5f, 0.0f)
};

static const float LINE_WIDTH = 0.1f;

[shader("vertex")]
VertexOut main(VertexIn input, uint vertex_id: SV_VertexID) {

    /* Get the quad corner for this thread */
    const float2 corner = quad[vertex_id];

    /* Get the world position of this corner */
    float3 world_pos = lerp(input.point_a, input.point_b, corner.y);
    const float3 line_dir = normalize(input.point_b - input.point_a);
    const float3 view_dir = normalize(render_view.origin.xyz - world_pos);

    float3 offset_dir = cross(line_dir, view_dir);
    float len = length(offset_dir);
    if (len < 0.001f) {
        offset_dir = cross(line_dir, float3(0.0f, 1.0f, 0.0f));
        len = length(offset_dir);
    }
    offset_dir /= len;
    world_pos += offset_dir * LINE_WIDTH * corner.x;

    /* Calculate the line in screen-space for checking the width */
    const float4 clip_pos = mul(render_view.world_to_clip, float4(world_pos, 1.0f));
    const float3 world_a = lerp(input.point_a, input.point_b, corner.y);
    const float3 world_b = world_a + offset_dir * LINE_WIDTH;
    const float4 clip_a = mul(render_view.world_to_clip, float4(world_a, 1.0f));
    const float4 clip_b = mul(render_view.world_to_clip, float4(world_b, 1.0f));
    const float2 screen_a = clip_a.xy / clip_a.w;
    const float2 screen_b = clip_b.xy / clip_b.w;

    /* Fade out the lines when they get too thin in screen */
    const float screen_width = length((screen_b - screen_a) * render_view.resolution * 0.5f);
    const float alpha = saturate(screen_width - 0.5f);

    VertexOut output;
    output.clip_pos = clip_pos;
    output.color = float4(1.0f, 0.3f, 0.3f, alpha);
    return output;
}
