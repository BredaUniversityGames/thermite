import lights;
import math;

StructuredBuffer<uint32_t> cascades_counters;
StructuredBuffer<uint32_t> cascades_bitmasks;
StructuredBuffer<uint32_t> light_grid;

StructuredBuffer<Light> lights;

static groupshared uint32_t group_buckets[32];

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID) {
    /* Retrieve current cascade and z coord */
    const uint32_t cascade_index = thread_id.z >> 4u; /* cascade_index = thread_id.z / 16u */
    const uint32_t z_coord = thread_id.z & 0b1111u;   /* z_coord = thread_id.z % 16u */

    /* Retrieve light count for this cascade */
    const uint32_t cascade_light_count = cascades_counters[cascade_index];

    /* 1. 16x16x16 -> 4x4x4 culling. Cull against 4x4x4 groups (64 threads) */
    /* Cascades Flat Bit array loop */
    for (uint32_t i = 0u; i < 32u; i++) {
        /* Retrieve bitmasks for this cascade */
        uint32_t cascade_bitmask = cascades_bitmasks[cascade_index * 32u + i];

        /* Loop over each bit in the bitmask (1 bit = 1 light) */
        while (cascade_bitmask != 0u) {
            /* Retrieve bit index */
            const uint32_t bit_index = firstbitlow(cascade_bitmask);

            /* Remove bit from mask */
            cascade_bitmask ^= (1u << bit_index);

            /* Get light */
            const uint32_t light_index = i * 32u + bit_index;
            const Light light = lights[light_index];

            /* Cull picked light against 4x4x4 group */
            {
                /* Create light aabb */
                Aabb light_aabb;
                light_aabb.max = light.origin + light.culling_radius;
                light_aabb.min = light.origin - light.culling_radius;

                /* Create group aabb */
            }
        }
    }
}
