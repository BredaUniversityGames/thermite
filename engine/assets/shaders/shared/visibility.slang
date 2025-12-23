module visibility;

import math;
import view;
import material;
import voxel;

/* Voxel visibility data. */
public struct VisData {
    public uint object_index;
    public uint voxel_index;
    /* Encodes the axis using 2 bits and the sign with 1 bit. (AAS) */
    public uint face_index;

    __init(uint a, uint b, uint c) {
        object_index = a; voxel_index = b; face_index = c;
    }
};

/* Voxel geometry data. */
public struct GeoData {
    public float depth;
    public float3 normal;
    public Material material;
};

/* Pack voxel visibility into two 32-bit unsigned integers. */
public [ForceInline] uint2 pack_visibility(const Hit hit) {
    if (hit.t == BIG_F32) return uint::maxValue;
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    const uint x = hit.voxel_index;
    const uint y = (hit.face_index << 16) | (hit.object_index & 0xFFFFu);
    return uint2(x, y);
}

/* Unpack voxel visibility. */
public [ForceInline] VisData unpack_visibility(const uint2 packed) {
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    return VisData(
        packed.y & 0xFFFFu,
        packed.x,
        packed.y >> 16
    );
}

/* Returns true if the visibility data doesn't contain a surface. */
public [ForceInline] bool null_visibility(const uint2 packed) {
    return packed.y == uint::maxValue;
}

/* Unpack voxel geometry from visibility data. */
public [ForceInline] GeoData unpack_geometry(const VisData vis, StructuredBuffer<VoxelObject> objects, const Ray ray) {
    GeoData data;

    // TODO: Use the normal and position to do a ray plane intersection, to find the depth.
    // TODO: Traverse down the 64Tree to find the material data.

    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal */
    const uint axis_index = vis.face_index & 0b11u;
    const uint axis_sign = vis.face_index >> 2;
    float3 local_normal = 0.0f;
    local_normal[axis_index] = (axis_sign == 0u) ? -1.0f : 1.0f;
    data.normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    return data;
}
