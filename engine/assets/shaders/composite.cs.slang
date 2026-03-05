import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;

/* Active render view */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;
/* Histroy Buffer. (Output) */
RWTexture2D<float4> history_buffer;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    float4 color = float4(0.0f);

    if (null_visibility(packed)) {
        color = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        const cache::Slot slot = cache::sample(macrofacet_cache, packed, render_view.frame_index);

        if (slot.key == cache::EMPTY_KEY) {
            render_target[thread_id] = float4(1.0f, 0.0f, 1.0f, 1.0f);
        } else {
            /* Generate a ray for our render view */
            const Ray ray = pinhole_ray(render_view, thread_id);

            /* Unpack visibility buffer data */
            const VisData vis = unpack_visibility(packed);
            const GeoData geo = unpack_geometry(vis, objects, ray);

            const float3 illuminance = slot.read_diffuse() * geo.material.albedo();
            color = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(illuminance))), 1.0f);
        }
    }

    /* TAA Resolve */
    const float4 output = float4(lerp(history_buffer[thread_id].rgb, color.rgb, 0.05f), 1.0f);
    render_target[thread_id] = output;
    history_buffer[thread_id] = output;
}
