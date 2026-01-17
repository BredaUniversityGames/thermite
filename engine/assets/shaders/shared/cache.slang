module cache;

namespace cache {

/* Fuse length for the cache read/write operations. */
public static const uint FUSE_LENGTH = 32u;
/* Empty slot key. */
public static const uint64_t EMPTY_KEY = 0ull;
/* Number of inactive frames after which to free a slot. */
public static const uint FREE_THRESHOLD = 60u;

/* Illuminance Cache Slot */
public struct Slot {
    public uint64_t key; /* 64-bit hash key */
    public uint illuminance_r;
    public uint illuminance_g;
    public uint illuminance_b;
    public uint samples; /* sample count & last sampled frame index */

    /* Quantization factor */
    static const float QFACTOR = 1e3f;

    public [ForceInline] float3 illuminance() {
        return float3(
            (float)illuminance_r * (1.0f / QFACTOR),
            (float)illuminance_g * (1.0f / QFACTOR),
            (float)illuminance_b * (1.0f / QFACTOR)
        ) / (samples & 0xFFFFFu);
    }

    public [ForceInline] [mutating] void accumulate(const float3 value) {
        const uint3 fxp = uint3(value * QFACTOR);
        InterlockedAdd(illuminance_r, fxp.r);
        InterlockedAdd(illuminance_g, fxp.g);
        InterlockedAdd(illuminance_b, fxp.b);
        InterlockedAdd(samples, 1u);
    }

    public [ForceInline] [mutating] void reset() {
        key = EMPTY_KEY;
        illuminance_r = 0u;
        illuminance_g = 0u;
        illuminance_b = 0u;
        samples = 0u;
    }

    public [ForceInline] [mutating] void set(const float3 value, const uint n) {
        const uint3 fxp = uint3(value * QFACTOR);
        illuminance_r = fxp.r;
        illuminance_g = fxp.g;
        illuminance_b = fxp.b;
        samples = n;
    }
};

} // cache
