/**
 * @name Illuminance Volume Update [1 thread per texel]
 * This pass updates the illuminance volume probes based on previously traced luminance.
 */
import math;
import volume;

/* Intermediate input buffer. */
Texture2D<float3> intermediate_buffer;
/* Probe output buffer. */
RWTexture2D<float3> probe_buffer;

[[vk::push_constant]] int cascade_index;

/* 8x8 luminance values in LDS. */
groupshared float gs_r[64];
groupshared float gs_g[64];
groupshared float gs_b[64];
/* 8x8 direction values in LDS. */
groupshared float gs_x[64];
groupshared float gs_y[64];
groupshared float gs_z[64];

/* Store luminance value into LDS. */
[ForceInline] void lds_store_lum(const int i, const float3 lum) { gs_r[i] = lum.r; gs_g[i] = lum.g; gs_b[i] = lum.b; }
/* Load luminance value from LDS. */
[ForceInline] float3 lds_load_lum(const int i) { return float3(gs_r[i], gs_g[i], gs_b[i]); }

/* Store direction value into LDS. */
[ForceInline] void lds_store_dir(const int i, const float3 dir) { gs_x[i] = dir.x; gs_y[i] = dir.y; gs_z[i] = dir.z; }
/* Load direction value from LDS. */
[ForceInline] float3 lds_load_dir(const int i) { return float3(gs_x[i], gs_y[i], gs_z[i]); }

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int2 texel_id: SV_GroupThreadID, int texel_i: SV_GroupIndex) {
    /* Find the UV coordinate of the current texel and calculate its direction */
    const float2 texel_uv = (float2(texel_id) + 0.5f) * (1.0f / PROBE_WIDTH);
    const float3 texel_dir = oct_decode(texel_uv);

    /* Each of the 8x8 threads in the current group loads one probe ray into LDS */
    lds_store_lum(texel_i, intermediate_buffer[thread_texel(thread_id, 0)]);
    lds_store_dir(texel_i, texel_dir);
    GroupMemoryBarrierWithGroupSync(); /* Wait for all threads to write to LDS */

    /* Integrate illuminance from the previously traced rays in LDS */
    float3 illuminance = 0.0f;
    [ForceUnroll] for (int i = 0; i < 64; ++i) {
        /* Load the direction of this ray from LDS */
        const float3 ray_dir = lds_load_dir(i);

        /* Compute the rays weight on the current texel and sum */
        const float ray_w = max(0.0f, dot(texel_dir, ray_dir));
        illuminance += ray_w * lds_load_lum(i);
    }

    /* Write out the illuminance into the illuminance volume */
    probe_buffer[thread_texel(thread_id, cascade_index)] = illuminance;
}
