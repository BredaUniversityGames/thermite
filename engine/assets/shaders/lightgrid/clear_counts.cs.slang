RWStructuredBuffer<uint32_t> light_grid;

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int3 thread_id: SV_DispatchThreadID, int3 group_id: SV_GroupID) {
    /* Retrieve current cascade and z coord */
    const uint32_t cascade_index = thread_id.z >> 4u; /* cascade_index = thread_id.z / 16u */
    const uint32_t z_coord = thread_id.z & 0b1111u;   /* z_coord = thread_id.z % 16u */

    const uint32_t cell_index = cascade_index * (16u * 16u * 16u) + (z_coord * 16u * 16u) + (thread_id.y * 16u) + thread_id.x;
    const uint32_t cell_base = cell_index * 64u;

    light_grid[cell_base] = 0u;
}
