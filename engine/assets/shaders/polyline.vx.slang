import view;

ConstantBuffer<RenderView> render_view;

/* Vertex shader input. */
struct VertexIn {
    float3 point_a : POSITION0;
    float3 point_b : POSITION1;
    float4 color : COLOR;
    float width : WIDTH;

    /* Returns true if this line has a world-space width. */
    [ForceInline] bool is_world_space() { return width < 0.0f; }
};

/* Vertex shader output. */
struct VertexOut {
    float4 clip_pos : SV_POSITION;
    float4 color : COLOR;
};

/* Clip a line again a frustum plane in clip-space. (returns true if line is outside the frustum) */
[ForceInline] bool clip_line_plane(float p, float d, inout float t_min, inout float t_max) {
    /* If the direction is parallel to the plane */
    if (d == 0.0f) return p < 0.0f; /* Line is outside if it is beyond the plane */

    /* Find the intersection time between the line and plane */
    const float t = -p / d;
    if (d < 0.0f) t_max = min(t_max, t); /* Entry */
    else t_min = max(t_min, t); /* Exit */

    /* Line is outside if tmax is larger than tmin */
    return t_min > t_max;
}

/* Clip a line from `clip_a` to `clip_b` in clip-space. (returns true if line is outside the frustum) */
[ForceInline] bool clip_line(inout float4 clip_a, inout float4 clip_b) {
    const float4 line = clip_b - clip_a;
    float t_min = 0.0f, t_max = 1.0f;

    /* Clip the line against each of the frustum planes */
    if (clip_line_plane(clip_a.x + clip_a.w, line.x + line.w, t_min, t_max)) return true; /* left plane   (x + w >= 0) */
    if (clip_line_plane(clip_a.w - clip_a.x, line.w - line.x, t_min, t_max)) return true; /* right plane  (w - x >= 0) */
    if (clip_line_plane(clip_a.y + clip_a.w, line.y + line.w, t_min, t_max)) return true; /* bottom plane (y + w >= 0) */
    if (clip_line_plane(clip_a.w - clip_a.y, line.w - line.y, t_min, t_max)) return true; /* top plane    (w - y >= 0) */
    if (clip_line_plane(clip_a.z, line.z, t_min, t_max)) return true;                     /* near plane   (z >= 0)     */
    if (clip_line_plane(clip_a.w - clip_a.z, line.w - line.z, t_min, t_max)) return true; /* far plane    (w - z >= 0) */

    /* Update the line endpoints in clip-space */
    const float4 clip_tmp = clip_a;
    clip_a = lerp(clip_tmp, clip_b, t_min);
    clip_b = lerp(clip_tmp, clip_b, t_max);
    return false;
}

/* Instanced vertices. */
static const float2 vertices[6] = {
    float2(0.0f, -0.5f),
    float2(1.0f, -0.5f),
    float2(1.0f,  0.5f),
    float2(0.0f, -0.5f),
    float2(1.0f,  0.5f),
    float2(0.0f,  0.5f)
};

[shader("vertex")]
VertexOut main(VertexIn input, uint vertex_id: SV_VertexID) {
    /* Get the vertex for this thread */
    const float2 vertex = vertices[vertex_id];
    const float2 resolution = render_view.resolution;

    /* Transform both world-space line endpoints into clip-space and clip them manually */
    float4 clip_a = mul(render_view.world_to_clip, float4(input.point_a, 1.0f));
    float4 clip_b = mul(render_view.world_to_clip, float4(input.point_b, 1.0f));
    if (clip_line(clip_a, clip_b)) return VertexOut(0.0f, 0.0f);

    /* Transform both clip-space line endpoints into "pixel-space" */
    const float2 pixel_a = resolution * (0.5f * (clip_a.xy / clip_a.w) + 0.5f);
    const float2 pixel_b = resolution * (0.5f * (clip_b.xy / clip_b.w) + 0.5f);

    /* Find our two basis vectors for drawing our line */
    const float2 basis_x = pixel_b - pixel_a;
    const float2 basis_y = normalize(float2(-basis_x.y, basis_x.x)); /* 2d cross-product */

    /* Generate a vertex of our line using the basis vectors */
    const float line_width = 2.0f;
    const float2 point_pixel = pixel_a + (basis_x * vertex.x) + (basis_y * line_width * vertex.y);

    /* Transform the vertex back into clip-space */
    const float2 point_uv = point_pixel / resolution;
    const float2 point_ndc = point_uv * 2.0f - 1.0f;
    const float2 clip_zw = lerp(clip_a.zw, clip_b.zw, vertex.x);
    const float4 point_clip = float4(clip_zw.y * point_ndc, clip_zw.x, clip_zw.y);

    /* World-space lines */
    if (input.is_world_space()) {
        /* TODO: Implement world-space lines */
        return VertexOut(0.0f, 0.0f);
    } else {
        return VertexOut(point_clip, input.color);
    }

    /* Convert line points into clip-space and perform manual near plane clipping */
    // float4 clip0 = mul(render_view.world_to_clip, float4(input.point_a, 1.0f));
    // float4 clip1 = mul(render_view.world_to_clip, float4(input.point_b, 1.0f));
    // // clip0 = clip_near_plane(clip0, clip1);
    // // clip1 = clip_near_plane(clip1, clip0);

    // /* Pick either point a or b based on the Z coordinate of the vertex */
    // const float4 clip = lerp(clip0, clip1, vertex.z);

    // /* Calculate the screen-space location of point a and b */
    // const float2 screen0 = float2(render_view.resolution) * (0.5f * clip0.xy / clip0.w + 0.5f);
    // const float2 screen1 = float2(render_view.resolution) * (0.5f * clip1.xy / clip1.w + 0.5f);

    // /* Find our two screen-space basis vectors */
    // const float2 x_basis = normalize(screen1 - screen0);
    // const float2 y_basis = float2(-x_basis.y, x_basis.x);

    // /* A negative line width indicates it should be in screen-space */
    // float line_width = 2.0f; // abs(input.width);
    // float4 line_color = input.color;

    // /* For positive line widths we use world-space units */
    // // if (input.width > 0.0f) line_width /= clip.w * 0.01f;

    // /* Fade out thin lines, based on <https://acegikmo.com/shapes/docs/#anti-aliasing> */
    // // if (line_width < 1.0f) {
    // //     line_color.a *= max(0.0f, line_width);
    // //     line_width = 1.0f;
    // // }

    // /* Calculate the final line point for this thread in clip-space */
    // const float2 pt_offset = line_width * (vertex.x * x_basis + vertex.y * y_basis);
    // const float2 pt = lerp(screen0 + pt_offset, screen1 + pt_offset, vertex.z);

    // VertexOut output;
    // output.clip_pos = float4(clip.w * ((2.0f * pt) / float2(render_view.resolution) - 1.0f), clip.z, clip.w);
    // output.color = line_color;
    // return output;
}
