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
