import view;
import visibility;
import math;
import voxel;
import cache;
import colorspace;
import bindless;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Environment map sampler. */
SamplerState envmap_sampler;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Luminance buffer. */
Texture2D<float3> lbuffer;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    if (null_visibility(packed)) {
        if (render_view.envmap_full_handle != 0u) {
            const float2 uv = dir_to_latlong(ray.dir);
            const float3 env = bindless::texture_sampled<Texture2D<float4>>(render_view.envmap_full_handle).SampleLevel(envmap_sampler, uv, 0).rgb;
            render_target[thread_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(env))), 1.0f);
        } else {
            render_target[thread_id] = float4(0.247f, 0.247f, 0.247f, 1.0f);
        }
    } else {
        const cache::Slot slot = cache::sample(macrofacet_cache, packed, render_view.frame_index);

        if (slot.key == cache::EMPTY_KEY) {
            render_target[thread_id] = float4(1.0f, 0.0f, 1.0f, 1.0f);
        } else {
            /* Unpack visibility buffer data */
            const VisData vis = unpack_visibility(packed);
            const GeoData geo = unpack_geometry(vis, objects, ray);

            const float3 illuminance = slot.read_diffuse() * geo.material.albedo();
            render_target[thread_id] = float4(cs::delinearize(cs::acescg_to_r709(cs::aces_filmic_tonemapping(illuminance))), 1.0f);
        }
    }
}
