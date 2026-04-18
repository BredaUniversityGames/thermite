module lightgrid;

public static constexpr uint32_t LIGHTS_PER_CELL = 64u; /* Actually only 63 lights since first uint32 in the cell is the light count for that cell */
public static constexpr uint32_t FIRST_CASCADE_BOUNDS = 32u;
public static constexpr uint32_t CASCADES_RESOLUTION = 16u;
public static constexpr uint32_t CASCADES_RESOLUTION2 = CASCADES_RESOLUTION * CASCADES_RESOLUTION;
public static constexpr uint32_t CASCADES_RESOLUTION3 = CASCADES_RESOLUTION * CASCADES_RESOLUTION * CASCADES_RESOLUTION;

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
    const uint32_t cascade_index = (uint32_t)max(0.0f, ceil(log2(max_axis) - 4.0f));
    if (cascade_index >= 8u)
        return { 0xFFFFFFFFu, 0u, 0u, 0u }; /* Out of bounds */

    /* Find which cell in the cascade the local point falls into */
    const uint32_t cascade_cell_extent = 1u << (cascade_index + 1u);
    const float3 cascade_point = floor(p / (float)cascade_cell_extent) + 8.0f;
    return { cascade_index, (uint32_t)cascade_point.x, (uint32_t)cascade_point.y, (uint32_t)cascade_point.z };
}