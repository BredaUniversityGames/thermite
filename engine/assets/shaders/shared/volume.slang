module volume;

namespace iv {

/* WARNING: Not all parts of the code actually respond correctly to the constants below! */

/* Width of a probe in rays/texels, probes are 2D. */
public const static int PROBE_WIDTH_BITS = 3; /* ^2 */
public const static int PROBE_WIDTH = 1 << PROBE_WIDTH_BITS;
public const static int PROBE_BORDER = 1;
public const static int PROBE_STRIDE = PROBE_WIDTH + PROBE_BORDER * 2;
/* Width of a cascade in probes, cascades are 3D. */
public const static int CASCADE_WIDTH_BITS = 4; /* ^3 */
public const static int CASCADE_WIDTH = 1 << CASCADE_WIDTH_BITS;
/* Extent of the smallest cascade in world-space units. */
public const static int CASCADE_EXTENT = 32; /* u */
public const static int CASCADE_HALF_EXTENT = CASCADE_EXTENT >> 1;
/* Total number of cascades. */
public const static int CASCADE_COUNT = 6;

/* Bit manipulation constants */
public const static int PROBE_WIDTH_MASK = PROBE_WIDTH - 1;
public const static int CASCADE_WIDTH_MASK = CASCADE_WIDTH - 1;
public const static int SLICE_WIDTH_BITS = CASCADE_WIDTH_BITS + PROBE_WIDTH_BITS;
public const static int CASCADE_HEIGHT_BITS = CASCADE_WIDTH_BITS + PROBE_WIDTH_BITS;

/* Size of the cascades texture atlas in texels */
public const static int2 ATLAS_SIZE = int2(
    CASCADE_WIDTH * PROBE_STRIDE * CASCADE_WIDTH,
    PROBE_STRIDE * CASCADE_WIDTH * CASCADE_COUNT
);

/* 3D position of the probe belonging to this thread index. */
public [ForceInline] float3 thread_probe(const int2 thread_id) {
    return float3(
        (thread_id.x >> PROBE_WIDTH_BITS) & CASCADE_WIDTH_MASK,
        thread_id.x >> SLICE_WIDTH_BITS,
        (thread_id.y >> PROBE_WIDTH_BITS) & CASCADE_WIDTH_MASK
    ) + 0.5f;
}

/* 2D UV of the ray belonging to this thread index. */
public [ForceInline] float2 thread_ray(const int2 thread_id) {
    return float2(thread_id.x & PROBE_WIDTH_MASK, thread_id.y & PROBE_WIDTH_MASK) + 0.5f;
}

/* 2D ID of the texel belonging to this thread index. */
public [ForceInline] int2 thread_texel(const int2 thread_id, const int cascade_index) {
    return int2(thread_id.x, (cascade_index << CASCADE_HEIGHT_BITS) | thread_id.y);
}

/* 2D ID of the top-left texel belonging to this probe id and cascade index. */
public [ForceInline] int2 sample_texel(const int3 probe_id, const int cascade_index) {
    return int2(
        probe_id.y * CASCADE_WIDTH * PROBE_STRIDE + probe_id.x * PROBE_STRIDE + PROBE_BORDER,
        cascade_index * CASCADE_WIDTH * PROBE_STRIDE + probe_id.z * PROBE_STRIDE + PROBE_BORDER
    );
}

/* Get the top-left texel belonging to the probe at a given view-space position. */
public [ForceInline] int2 probe_uv(const float3 pos) {
    const int3 probe_id = clamp(int3(pos / (CASCADE_EXTENT / CASCADE_WIDTH)) + (CASCADE_WIDTH / 2), 0, CASCADE_WIDTH);
    return sample_texel(probe_id, 0);
}

/* Size of the froxel grid in froxels. */
static const int3 FROXEL_GRID_SIZE = int3(160 / 4, 90 / 4, 64 / 4);

/* Get the center position of a froxel in world-space. */
public [ForceInline] float3 froxel_to_world(const int3 froxel, const float4x4 clip_to_world, const float near_z, const float far_z) {
    /* Calculate the normalized device z coordinate (exponential distribution) */
    const float z_center = near_z * pow(far_z / near_z, (float(froxel.z) + 0.5f) / FROXEL_GRID_SIZE.z);
    const float z_ndc = (far_z * near_z / z_center - far_z) / (near_z - far_z);

    /* Calculate the normalize device x and y coordinates */
    const float x_ndc = (float(froxel.x) + 0.5f) / (FROXEL_GRID_SIZE.x * 0.5f) - 1.0f;
    const float y_ndc = (float(froxel.y) + 0.5f) / (FROXEL_GRID_SIZE.y * 0.5f) - 1.0f;

    /* Transform from clip-space to world-space */
    const float4 view = mul(clip_to_world, float4(x_ndc, y_ndc, z_ndc, 1.0f));
    return view.xyz / view.w;
}

/* Get the UVW of the froxel volume at a given world-space position. */
public [ForceInline] float3 froxel_uvw(const float3 pos, const float4x4 world_to_clip, const float near_z, const float far_z) {
    /* Transform world-space to clip-space, then NDC */
    const float4 clip = mul(world_to_clip, float4(pos, 1.0f));
    const float3 ndc = clip.xyz / clip.w;

    /* Invert the NDC x,y mapping */
    const float u = ndc.x * 0.5f + 0.5f;
    const float v = ndc.y * 0.5f + 0.5f;

    /* Recover linear depth from NDC z, then invert the exponential distribution */
    const float z_linear = far_z * near_z / (ndc.z * (near_z - far_z) + far_z);
    const float w = log(z_linear / near_z) / log(far_z / near_z);
    return float3(u, v, w);
}

}
