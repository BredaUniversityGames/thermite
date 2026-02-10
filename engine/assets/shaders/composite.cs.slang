import view;
import voxel;
import visibility;
import colorspace;
import cache;
import math;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */ 
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    if (null_visibility(facet_key)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Generate a ray for our render view */
        const Ray ray = pinhole_ray(render_view, thread_id);

        /* Unpack visibility buffer data */
        const VisData vis = unpack_visibility(facet_key);
        const GeoData geo = unpack_geometry(vis, objects, ray);

        /* Sample the illuminance cache */
        const float3 illuminance = cache::sample(macrofacet_illuminance_cache, facet_key, render_view.frame_index);
        render_target[thread_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(illuminance))), 1.0f); /* TEMP: Remove tonemapping! */

        /* Calculate some fake shading for the view */
        const float fake_shading = clamp(dot(-ray.dir, geo.normal), 0.2f, 0.8f);
        render_target[thread_id] = float4(geo.material.albedo * fake_shading, 1.0f);
    }
}
