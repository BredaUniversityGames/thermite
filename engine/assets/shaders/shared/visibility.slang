module visibility;

import math;
import view;
import material;
import voxel;
import svt64;
import bindless;

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

/* Expands one dimensional voxel index into three dimensional voxel position. */
[ForceInline] uint3 expand_voxel_index(const uint voxel_index) {
    return uint3(voxel_index & 0b1111111111u, (voxel_index >> 10) & 0b1111111111u, voxel_index >> 20);
}

/* Unpack voxel geometry from visibility data. */
public [ForceInline] GeoData unpack_geometry(const VisData vis, StructuredBuffer<VoxelObject> objects, const Ray ray) {
    GeoData data;
    const VoxelObject object = objects[vis.object_index];

    /* Unpack the geometry normal (1b sign | 2b axis) */
    float3 local_normal = 0.0f;
    local_normal[vis.face_index & 0b11u] = (vis.face_index >> 2) == 0u ? -1.0f : 1.0f;
    data.normal = normalize(mul(object.local_to_world, float4(local_normal, 0.0f)).xyz);

    /* Unpack the voxel position index (1D -> 3D) */
    const uint3 voxel_index = expand_voxel_index(vis.voxel_index);

    /* Calculate the voxel face position in world-space */
    const float3 voxel_pos = (float3(voxel_index) + 0.5f + local_normal * 0.5f) * UNITS_PER_VOXEL;
    const float3 half_extent = float3(object.size_rcp_width.xyz) * 0.5f * UNITS_PER_VOXEL;
    const float3 world_voxel = mul(object.local_to_world, float4(voxel_pos - half_extent, 1.0f)).xyz;

    /* Find the depth of the geometry */
    data.depth = dot(world_voxel - ray.origin, data.normal) / dot(data.normal, ray.dir);

    /* Find the voxel data pointer by traversing the SVT64 */
    const StructuredBuffer<Svt64Node> nodes = bindless::storage_buffer<StructuredBuffer<Svt64Node>>(object.blas_handle);
    const uint voxel_ptr = fetch_svt64(nodes, object.tree_depth, voxel_index);

    /* Fetch the voxel palette index and it's material from the palette */
    const StructuredBuffer<uint> voxels = bindless::storage_buffer<StructuredBuffer<uint>>(object.voxels_handle);
    const StructuredBuffer<Material> palette = bindless::storage_buffer<StructuredBuffer<Material>>(object.palette_handle);
    const uint palette_index = (voxels[voxel_ptr >> 2u] >> ((4u - (voxel_ptr % 4u)) << 3u)) & 0xFFu;
    data.material = palette[palette_index];
    
    return data;
}
