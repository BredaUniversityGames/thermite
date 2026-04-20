module lightgrid;

namespace lgrid {

    public static constexpr uint32_t MAX_CASCADES = 6u;
    public static constexpr uint32_t LIGHTS_PER_CELL = 64u; /* Actually only 63 lights since first uint32 in the cell is the light count for that cell */
    public static constexpr uint32_t FIRST_CASCADE_BOUNDS = 32u;
    public static constexpr uint32_t CASCADES_RESOLUTION = 16u;
    public static constexpr uint32_t CASCADES_RESOLUTION2 = CASCADES_RESOLUTION * CASCADES_RESOLUTION;
    public static constexpr uint32_t CASCADES_RESOLUTION3 = CASCADES_RESOLUTION * CASCADES_RESOLUTION * CASCADES_RESOLUTION;

    public static constexpr float CASCADE_LOG_OFFSET = log2((float)FIRST_CASCADE_BOUNDS) - 1.0f; /* log2(half_extent) = log2(32) - 1 = 4 */
    public static constexpr float GRID_HALF_RES = (float)CASCADES_RESOLUTION / 2.0f;             /* 16 / 2 = 8 */

    public struct CascadeIndex {
        public uint32_t cascade = 0u;
        public uint32_t x = 0u;
        public uint32_t y = 0u;
        public uint32_t z = 0u;
    };

    /* Index light cascades using view-space point `p`. */
    public inline CascadeIndex index_cascades(const float3 p) {
        /* Calculate the cascade index */
        const float max_axis = max(max(abs(p.x), abs(p.y)), abs(p.z));
        const uint32_t cascade_index = (uint32_t)max(0.0f, ceil(log2(max_axis) - CASCADE_LOG_OFFSET));
        if (cascade_index >= MAX_CASCADES)
            return { 0xFFFFFFFFu, 0u, 0u, 0u }; /* Out of bounds */

        /* Find which cell in the cascade the local point falls into */
        const uint32_t cascade_cell_extent = 1u << (cascade_index + 1u);
        const float3 cascade_point = floor(p / (float)cascade_cell_extent) + GRID_HALF_RES;
        return { cascade_index, (uint32_t)cascade_point.x, (uint32_t)cascade_point.y, (uint32_t)cascade_point.z };
    }

}
