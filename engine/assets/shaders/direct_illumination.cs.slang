/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per macrofacet]
 * ...
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;

/* TEMPORARY */
static const float3 SUN_LUMINANCE = float3(1.0f, 0.7f, 0.1f) * 10.0f;
static const float3 AO_LUMINANCE = float3(0.2f, 0.5f, 1.0f);
static const float3 LIGHT_DIR = normalize(float3(1.0f, 1.5f, 1.1f));
/* TEMPORARY */

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[thread_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Unpack visibility data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    const float3 hit_point = ray.origin + ray.dir * geo.depth;

    const uint n = thread_id.x + thread_id.y * render_view.resolution.x + render_view.frame_index * render_view.resolution.x * render_view.resolution.y;
    
    float3 dir, ill;
    // if (r1(n % 512) < 0.5f) {
    dir = sample_hemisphere(geo.normal, r2((n + 1) % 512));
    ill = float3(0.3f, 0.3f, 0.3f); // AO_LUMINANCE;
    // } else {
    //     dir = sample_cone(LIGHT_DIR, cos(0.05f), r2((n + 1) % 512));
    //     ill = SUN_LUMINANCE * max(0.0f, dot(geo.normal, dir));
    // }

    /* Trace light occlusion */
    const Ray occlusion_ray = custom_ray(hit_point + geo.normal * 0.001f, dir);
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 1e30f);
    float3 illuminance = 0.0f;
    if (t >= 1e30f) {
        illuminance += ill;
    }

    /* Insert the sample into the illuminance cache */
    cache::insert(macrofacet_illuminance_cache, facet_key, illuminance);
}
