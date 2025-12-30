import view;
import voxel;
import visibility;
import material;
import math;
import bvh2;
import cache;

/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Macro-facet Buffer (Input) */
/* First 4 uints are indirect dispatch args, after that uint2 keys. */
RWStructuredBuffer<uint> macrofacets;
/* Illuminance Cache Info (Input) */
ConstantBuffer<CacheInfo> cache_info;
/* Illuminance Cache (Output) */
RWStructuredBuffer<CacheEntry> cache_entries;

/* TEMPORARY */
static const float3 LIGHT_COLOR = float3(1.0f, 0.1f, 0.1f);
static const float3 LIGHT_POS = float3(2.0f, 12.0f, 4.0f);
static const float LIGHT_STRENGTH = 256.0f;
/* TEMPORARY */

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Load visibility buffer data */
    const uint64_t packed = asuint64(uint2(macrofacets[4 + thread_id * 2], macrofacets[4 + thread_id * 2 + 1]));

    /* Unpack visibility buffer data */
    const VisData vis = unpack_visibility(packed);
    // const GeoData geo = unpack_geometry(vis, object_data);

    // TODO: Reset occupancy counter inside cache. (for next frame)
    const uint occupancy = cache::reset_occupancy(cache_entries, cache_info, packed);

    macrofacets[0] = 0u;
    macrofacets[1] = 1u;
    macrofacets[2] = 1u;
    macrofacets[3] = 0u;

    // TODO: Reset indirect dispatch arguments for macrofacets. (for next frame)
    // TODO: Unpack geometry info, get position and normal of voxel facet.
    // TODO: Evaluate lighting for voxel facet.
    // TODO: Write evaluated lighting to illuminance cache.
}
