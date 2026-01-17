/**
 * @name Recycling [1 thread per hash cell]
 * ...
 */
import view;

static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
    uint samples;
};

/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<HashCell> macrofacet_illuminance_cache;

[shader("compute")]
[numthreads(128, 1, 1)]
void main(int thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (thread_id >= 10000000u) return;

    /* Skip empty slots */
    if (macrofacet_illuminance_cache[thread_id].key == EMPTY_KEY) return;

    /* Get the sample count for this slot */
    const uint sample_count = macrofacet_illuminance_cache[thread_id].samples;
    if (sample_count == 1u) {
        macrofacet_illuminance_cache[thread_id].key = EMPTY_KEY;
        return;
    }

    if (sample_count >= 512u) {
        /* Evaluate the mean value of this slot */
        const float value = (float)macrofacet_illuminance_cache[thread_id].value * 1e-3f;
        const float mean = value / sample_count;

        /* Reduce sample count back to 1 */
        macrofacet_illuminance_cache[thread_id].samples = 256u;
        macrofacet_illuminance_cache[thread_id].value = (uint)(mean * 256.0f * 1e3f);
    }
}
