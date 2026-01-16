import view;
import visibility;
import math;
import voxel;
import color;
import debug;

static const uint FUSE_LENGTH = 32u;
static const uint64_t EMPTY_KEY = 0ull;

struct HashCell {
    uint64_t key;
    uint value;
    uint state;
};

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
StructuredBuffer<HashCell> macrofacet_illuminance_cache;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) {
        const float3 illuminance = srgb_to_acescg(float3(0.2f, 0.5f, 1.0f));
        render_target[thread_id] = float4(delinearize_srgb(acescg_to_linear_srgb(illuminance)), 1.0f);
        return;
    }
    /* Unpack the key into its voxel components */
    const VisData v = unpack_visibility(facet_key);

    /* Evaluate the voxel hash function */
    const uint capacity = 10000000u;
    uint slot = voxel_hash(v.voxel_index, v.object_index, v.face_index) % capacity;

    /* Find the macrofacet */
    for (uint fuse = 0u; fuse < FUSE_LENGTH; ++fuse) {
        /* Exit if we found a slot which matches our key */
        if (macrofacet_illuminance_cache[slot].key == facet_key) {
            // const Reservoir r = decode_reservoir(macrofacet_illuminance_cache[slot].state, macrofacet_illuminance_cache[slot].value);
            // render_target[thread_id] = float4(decode_rgb9e5(r.y) * r.w_sum / max(r.m, 0.0001f), 1.0f);
            const float3 illuminance = decode_rgb16(uint2(macrofacet_illuminance_cache[slot].value, macrofacet_illuminance_cache[slot].state));
            // render_target[thread_id] = float4(1.0f - 1.0f / pow(1.0f + illuminance, float3(2.5f)), 1.0f);  // float4(decode_rgb9e5(macrofacet_illuminance_cache[slot].value), 1.0f);
            render_target[thread_id] = float4(delinearize_srgb(acescg_to_linear_srgb(illuminance)), 1.0f);
            return;
        }

        /* Move to the next hash cache slot */
        slot = (slot + 1u) % capacity;
    }

    render_target[thread_id] = float4(1.0f, 0.0f, 0.0f, 1.0f);
}
