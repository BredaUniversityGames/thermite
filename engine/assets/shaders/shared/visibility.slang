module visibility;

import math;
import view;

/* Voxel visibility data. */
public struct VisData {
    public uint object_index;
    public uint voxel_index;
    public uint face_index;

    __init(uint a, uint b, uint c) {
        object_index = a; voxel_index = b; face_index = c;
    }
};

/* Pack voxel visibility into two 32-bit unsigned integers. */
public [ForceInline] uint2 pack_visibility(Hit hit) {
    if (hit.t == BIG_F32) return uint::maxValue;
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    const uint x = hit.voxel_index;
    const uint y = (hit.face_index << 16) | (hit.object_index & 0xFFFFu);
    return uint2(x, y);
}

/* Unpack voxel visibility. */
public [ForceInline] VisData unpack_visibility(uint2 packed) {
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    return VisData(
        packed.y & 0xFFFFu,
        packed.x,
        packed.y >> 16
    );
}

/* Returns true if the visibility data doesn't contain a surface. */
public [ForceInline] bool null_visibility(uint2 packed) {
    return packed.y == uint::maxValue;
}
