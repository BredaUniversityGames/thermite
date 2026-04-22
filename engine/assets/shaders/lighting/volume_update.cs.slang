/**
 * @name Illuminance Volume Update [1 thread per texel]
 * This pass updates the illuminance volume probes based on previously traced luminance.
 */
import math;
import volume;

/* Intermediate input buffer. */
Texture2D<float3> intermediate_buffer;
Texture2D<float3> dir_buffer;
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

/* Map a texel index to a unique probe border texel. */
int2 map_to_border(const int i) {
    if (i < iv::PROBE_STRIDE) return int2(i, 0);
    else if (i < iv::PROBE_STRIDE * 2) return int2(i - iv::PROBE_STRIDE, iv::PROBE_STRIDE - 1);
    else if (i < iv::PROBE_STRIDE * 2 + iv::PROBE_STRIDE - 2) return int2(0, i - iv::PROBE_STRIDE * 2 + 1);
    else return int2(iv::PROBE_STRIDE - 1, i - (iv::PROBE_STRIDE * 3 - 2) + 1);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID, int2 texel_id: SV_GroupThreadID, int texel_i: SV_GroupIndex) {
    /* Find the UV coordinate of the current texel and calculate its direction */
    const float2 texel_uv = (float2(texel_id) + 0.5f) * (1.0f / iv::PROBE_WIDTH);
    const float3 texel_dir = oct_decode(texel_uv);

    /* Each of the 8x8 threads in the current group loads one probe ray into LDS */
    lds_store_lum(texel_i, intermediate_buffer[iv::thread_texel(thread_id, 0)]);
    lds_store_dir(texel_i, dir_buffer[iv::thread_texel(thread_id, 0)]);
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
    const int3 probe_id = int3(iv::thread_probe(thread_id));
    const int2 probe_origin = int2(
        probe_id.y * iv::CASCADE_WIDTH * iv::PROBE_STRIDE + probe_id.x * iv::PROBE_STRIDE + iv::PROBE_BORDER,
        cascade_index * iv::CASCADE_WIDTH * iv::PROBE_STRIDE + probe_id.z * iv::PROBE_STRIDE + iv::PROBE_BORDER
    );
    /* Interpolate history */
    const float3 history = probe_buffer[probe_origin + texel_id];
    illuminance = lerp(illuminance, history, 0.9f);
    probe_buffer[probe_origin + texel_id] = illuminance;
    GroupMemoryBarrierWithGroupSync();

    /* Store illuminance into LDS for filling the probe border */
    lds_store_lum(texel_i, illuminance);
    GroupMemoryBarrierWithGroupSync();
    if (texel_i >= 36) return; /* We only need 36 threads for the border */

    /* Map the texel index to the probe border */
    const int2 border_texel = map_to_border(texel_i) - iv::PROBE_BORDER;

    /* Mirror the border texel to find the source texel to copy */
    const bool flip_x = border_texel.x < 0 || border_texel.x >= iv::PROBE_WIDTH;
    const bool flip_y = border_texel.y < 0 || border_texel.y >= iv::PROBE_WIDTH;
    const int2 src_texel = int2(
        clamp(select(flip_y, iv::PROBE_WIDTH - 1 - border_texel.x, border_texel.x), 0, iv::PROBE_WIDTH - 1),
        clamp(select(flip_x, iv::PROBE_WIDTH - 1 - border_texel.y, border_texel.y), 0, iv::PROBE_WIDTH - 1)
    );

    /* Copy the source texel into the border texel */
    probe_buffer[probe_origin + border_texel] = lds_load_lum(src_texel.y * iv::PROBE_WIDTH + src_texel.x);
}
