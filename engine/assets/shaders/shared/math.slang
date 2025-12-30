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
