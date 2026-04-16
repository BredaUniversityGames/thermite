import lights;
import math;

StructuredBuffer<uint32_t> cascades_bitmasks;

StructuredBuffer<Light> lights;

RWStructuredBuffer<uint32_t> light_grid;

static groupshared uint32_t group_buckets[32];

static constexpr uint32_t FIRST_CASCADE_BOUNDS = 32u;

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID, int3 group_id: SV_GroupID) {
    /* Retrieve current cascade and z coord */
    const uint32_t cascade_index = thread_id.z >> 4u; /* cascade_index = thread_id.z / 16u */
    const uint32_t z_coord = thread_id.z & 0b1111u;   /* z_coord = thread_id.z % 16u */

    /* Compute current cascade aabb */
    const uint32_t cascade_half_extent = (FIRST_CASCADE_BOUNDS << cascade_index) / 2u;
    const float3 center = float3(0.0f, 0.0f, 0.0f);
    const float3 cascade_max = { center.x + cascade_half_extent, center.y + cascade_half_extent, center.z + cascade_half_extent };
    const float3 cascade_min = { center.x - cascade_half_extent, center.y - cascade_half_extent, center.z - cascade_half_extent };
    Aabb cascade_aabb;
    cascade_aabb.min = cascade_min;
    cascade_aabb.max = cascade_max;

    const uint32_t group_extent = cascade_half_extent / 2u; /* cascade_extent / 4u */
    Aabb group_aabb;
    group_aabb.min = cascade_min + group_extent * uint3(group_id.x, group_id.y, group_id.z % 4);
    group_aabb.max = group_aabb.min + group_extent;

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
            /* Create light aabb */
            Aabb light_aabb;
            light_aabb.max = light.origin + light.culling_radius;
            light_aabb.min = light.origin - light.culling_radius;

            /* Update group bitmask */
            if (group_aabb.overlap(light_aabb)) {
                const uint32_t group_bit_index = (uint32_t)light_index & 0b11111u; /* light_index % 32 */
                group_buckets[i] |= 1 << group_bit_index;
            }
        }
    }

    /* Wait for all groups to finish */
    GroupMemoryBarrierWithGroupSync();

    /* Compute cell aabb */
    const uint32_t cell_extent = group_extent / 4u;
    Aabb cell_aabb;
    cell_aabb.min = cascade_min + cell_extent * uint3(thread_id.x, thread_id.y, z_coord);
    cell_aabb.max = cell_aabb.min + cell_extent;

    /* 2. 4x4x4 -> 1x1x1 culling. Cull against individual cells in the light grid (1 thread = 1 cell) */
    for (uint32_t i = 0u; i < 32u; i++) {
        uint32_t group_bitmask = group_buckets[i];

        /* Loop over each bit in the group bitmask */
        while (group_bitmask != 0u) {
            /* Retrieve bit index */
            const uint32_t bit_index = firstbitlow(group_bitmask);

            /* Remove bit from mask */
            group_bitmask ^= (1u << bit_index);

            /* Get light */
            const uint32_t light_index = i * 32u + bit_index;
            const Light light = lights[light_index];

            /* Cull picked light against cell (thread) */
            /* Create light aabb */
            Aabb light_aabb;
            light_aabb.max = light.origin + light.culling_radius;
            light_aabb.min = light.origin - light.culling_radius;

            /* Insert light index into light_grid */
            if (cell_aabb.overlap(light_aabb)) {
                /* Retrieve cell index */
                const uint32_t cell_index = cascade_index * (16u * 16u * 16u) + (z_coord * 16u * 16u) + (thread_id.y * 16u) + thread_id.x;
                const uint32_t cell_base = cell_index * 64u;

                /* Increment counter */
                uint32_t slot;
                InterlockedAdd(light_grid[cell_base], 1u, slot);

                /* Append light grid */
                if (slot < 63u) /* Max 63 lights per cell */ 
                    light_grid[cell_base + 1u + slot] = light_index;
            }
        }
    }
}
