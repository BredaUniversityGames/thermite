import cache;
import voxel;
import visibility;
import view;
import math;

/* Active render view (Input) */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer (Input) */
RWTexture2D<uint2> vbuffer;
/* Illuminance Cache Info (Input) */
ConstantBuffer<CacheInfo> cache_info;
/* Illuminance Cache (Output) */
RWStructuredBuffer<CacheEntry> cache_entries;
/* Macro-facet Buffer (Output) */
/* First 4 uints are indirect dispatch args, after that uint2 keys. */
RWStructuredBuffer<uint> macrofacets;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(packed)) return;

    /* Increment cache occupancy, and register new macrofacet if this was its first occurrence */
    if (cache::inc_occupancy(cache_entries, cache_info, packed, render_view.frame_index)) {
        /* Increment the atomic macrofacet counter */
        uint facet_index = 0u;
        InterlockedAdd(macrofacets[3], 1u, facet_index);

        /* Update the indirect dispatch args if required */
        if ((macrofacets[0] * 128u) < facet_index) macrofacets[0] = (facet_index + 127u) / 128u;

        /* Insert the macrofacet visibility data */
        const uint2 seperated = asuint2(packed);
        macrofacets[4u + facet_index * 2u + 0u] = seperated.x;
        macrofacets[4u + facet_index * 2u + 1u] = seperated.y;
    }
}
