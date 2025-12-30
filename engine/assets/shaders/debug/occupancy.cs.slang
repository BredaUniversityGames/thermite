import visibility;
import cache;
import math;
import debug;

/* Visibility buffer. (Input) */
RWTexture2D<uint2> vbuffer;
/* Illuminance Cache Info (Input) */
ConstantBuffer<CacheInfo> cache_info;
/* Illuminance Cache (Output) */
RWStructuredBuffer<CacheEntry> cache_entries;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Read the occupancy of the cache */
        const uint occupancy = cache::read(cache_entries, cache_info, packed).occupancy;
        render_target[thread_id] = float4(heat_color((float)occupancy * 0.01f), 1.0f);
    }
}
