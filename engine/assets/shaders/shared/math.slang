module math;

/* Large floating point constant. */
public static const float BIG_F32 = 1e30f;

/* Number of voxels per unit in world-space. */
public static const uint VOXELS_PER_UNIT = 10u;
/* Number of world-space units per voxel. */
public static const float UNITS_PER_VOXEL = 1.0f / VOXELS_PER_UNIT;

/* Returns true if the given bit `i` in `x` is set. */
public [ForceInline] bool bit_set(const uint x, const uint i) {
    const uint mask = 1u << i;
    return (x & mask) != 0u;
}

/* Convert a uint64 into two uint32s. */
public [ForceInline] uint2 asuint2(const uint64_t i) {
    return uint2((uint)i, (uint)(i >> 32));
}

/* Convert two uint32s into a single uint64. */
public [ForceInline] uint64_t asuint64(const uint2 i) {
    return ((uint64_t)i.y << 32) | i.x;
}

/* Encode a HDR color value packed as RGB9E5. */
public uint encode_rgb9e5(const float3 rgb) {
    const float kmax = 65408.0f; /* 1.FF x 2^+15 */
    const float kmin = .000015f; /* 1.00 x 2^-16 */

    /* Clamp the RGB values */
    const float sr = clamp(rgb.x, 0.0f, kmax);
    const float sg = clamp(rgb.y, 0.0f, kmax);
    const float sb = clamp(rgb.z, 0.0f, kmax);

    /* Find the maximum channel */
    const float maxc = max(max(kmin, sr), max(sg, sb));
    const uint bias = (asuint(maxc) + 0x07804000u) & 0x7F800000u;
    const float fbias = asfloat(bias);

    /* Finally shift the bits into their places */
    const float pr = sr + fbias;
    const float pg = sg + fbias;
    const float pb = sb + fbias;
    const uint pe = (bias << 4u) + 0x10000000u;
    return pe | (asuint(pb) << 18u) | (asuint(pg) << 9u) | (asuint(pr) & 0x1FFu);
}

/* Decode a HDR color value packed as RGB9E5. */
public float3 decode_rgb9e5(const uint p) {
    const float x = p & 0x1FFu, y = (p >> 9u) & 0x1FFu, z = (p >> 18u) & 0x1FFu;
    const int e = (int)(p >> 27u) - 24;
    const float r = ldexp(x, e), g = ldexp(y, e), b = ldexp(z, e);
    return float3(r, g, b);
}

public uint2 encode_rgb16(const float3 rgb) {
    const uint px = f32tof16(rgb.r) | (f32tof16(rgb.g) << 16);
    const uint py = f32tof16(rgb.b);
    return uint2(px, py);
}

public float3 decode_rgb16(const uint2 p) {
    const float r = f16tof32(p.x & 0xFFFFu);
    const float g = f16tof32(p.x >> 16);
    const float b = f16tof32(p.y & 0xFFFFu);
    return float3(r, g, b);
}

public struct Basis {
    public float3 a;
    public float3 b;
};

/* Create two orthonormal basis vectors for a given normal vector. */
public [ForceInline] Basis orthonormal_basis(const float3 n) {
    const float sign = n.z >= 0.0f ? 1.0f : -1.0f;
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;

    return Basis(
        float3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x),
        float3(b, sign + n.y * n.y * a, -n.y)
    );
}

public struct Reservoir {
    public uint y; /* Current sample. */
    public float m; /* Number of samples seen */
    public float w; /* Cached weight for reuse */
    public float w_sum; /* Sum of weights */
};

public uint encode_reservoir(const Reservoir r) {
    return f32tof16(r.m) | (f32tof16(r.w_sum) << 16);
}

public Reservoir decode_reservoir(const uint p, const uint y) {
    return Reservoir(y,
        f16tof32(p & 0xFFFFu),
        1.0f, /* weight of 1.0 */
        f16tof32(p >> 16)
    );
}
