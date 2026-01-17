/**
 * @name Recycling [1 thread per hash cell]
 * ...
 */
import math;
import view;
import cache;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;

static const uint NORM_THRESHOLD = 512u;
static const uint BASE_COUNT = 256u;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= 10000000u) return;

    /* Skip empty slots */
    if (macrofacet_illuminance_cache[thread_id].key == cache::EMPTY_KEY) return;

    /* Free this slot if it has gone unused for a while */
    // const uint test = ((macrofacet_illuminance_cache[thread_id].samples >> 20) + cache::FREE_THRESHOLD) & 0xFFFu;

    const uint qframe = render_view.frame_index & 0xFFFu;
    const uint qsampled = macrofacet_illuminance_cache[thread_id].samples >> 20;

    int diff = ((qframe - qsampled) << 20) >> 20;

    if (diff > cache::FREE_THRESHOLD) {
        macrofacet_illuminance_cache[thread_id].reset();
        return;
    }

    const uint sample_count = macrofacet_illuminance_cache[thread_id].samples & 0xFFFFFu;
    if (sample_count > NORM_THRESHOLD) {
        /* Evaluate the mean value of this slot */
        const float3 mean = macrofacet_illuminance_cache[thread_id].illuminance();

        /* Reduce sample count back to 1 */
        macrofacet_illuminance_cache[thread_id].set(mean * BASE_COUNT, BASE_COUNT);
    }
}
