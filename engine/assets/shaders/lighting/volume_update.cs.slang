/**
 * @name Illuminance Volume Update [1 thread per texel]
 * This pass updates the illuminance volume probes based on previously traced luminance.
 */
import math;

/* Intermediate input buffer. */
Texture3D<float3> intermediate_buffer;
/* Probe output buffer. */
RWTexture3D<float3> probe_buffer;

[[vk::push_constant]] uint cascade_index;

/* Octahedral probe patch size in pixels. */
const static int2 PROBE_SIZE = int2(8, 8);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 texel_id: SV_GroupThreadID, int3 probe_id: SV_GroupID) {
    /* Find the UV coordinate of the current texel and calculate its direction */
    const float2 texel_uv = (float2(texel_id) + 0.5f) / PROBE_SIZE;
    const float3 texel_dir = oct_decode(texel_uv);

    /* Integrate illuminance from the previously traced rays */
    float3 illuminance = 0.0f;
    const int3 probe_base = int3(probe_id.xy * PROBE_SIZE, probe_id.z);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const int2 ray_id = int2(x, y);

            /* Find the UV coordinate of the current ray and calculate its direction */
            const float2 ray_uv = (float2(ray_id) + 0.5f) / PROBE_SIZE;
            const float3 ray_dir = oct_decode(ray_uv);

            /* Calculate the ray's weight on the current texel and sum */
            const float ray_w = max(0.0f, dot(texel_dir, ray_dir));
            illuminance += ray_w * intermediate_buffer[probe_base + int3(ray_id, 0)];
        }
    }

    /* Write out the illuminance into the illuminance volume */
    probe_buffer[int3(probe_base.x + texel_id.x, probe_base.y + texel_id.y, probe_base.z + cascade_index * 16)] = illuminance;
}
