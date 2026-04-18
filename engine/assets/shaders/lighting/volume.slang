module volume;

/* Width of a probe in rays/texels, probes are 2D. */
public const static int PROBE_WIDTH_BITS = 3; /* ^2 */
public const static int PROBE_WIDTH = 1 << PROBE_WIDTH_BITS;
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
    CASCADE_WIDTH * PROBE_WIDTH * CASCADE_WIDTH,
    PROBE_WIDTH * CASCADE_WIDTH * CASCADE_COUNT
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
        (probe_id.y << SLICE_WIDTH_BITS) | (probe_id.x << PROBE_WIDTH_BITS),
        (cascade_index << CASCADE_HEIGHT_BITS) | (probe_id.z << PROBE_WIDTH_BITS)
    );
}
