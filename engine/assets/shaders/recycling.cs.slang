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

    /* Find the number of frames between the current and last sampled frame */
    const uint inactivity = macrofacet_illuminance_cache[thread_id].inactivity(render_view.frame_index);

    /* If this slot has been inactive for too long, reset it so it can be re-used */
    if (inactivity > cache::INACTIVE_THRESHOLD) {
        macrofacet_illuminance_cache[thread_id].reset();
        return;
    }

    /* Normalize the illuminance in the cache slot */
    macrofacet_illuminance_cache[thread_id].normalize();
}
