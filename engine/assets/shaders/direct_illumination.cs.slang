/**
 * @name Macrofacet Direct Illumination (DI) [1 thread per pixel]
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import visibility;
import voxel;
import bvh2;
import cache;
import lights;

struct DiSettings {
    /* 0 = FullRate, 1 = HalfRate, 2 = QuarterRate */
    uint shading_rate;
};

/* Active render & scene view. */
ConstantBuffer<RenderView> render_view;
ConstantBuffer<SceneView> scene_view;
/* Settings buffer. */
ConstantBuffer<DiSettings> settings;
/* Voxel objects buffers. */
StructuredBuffer<float4> tlas_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> objects;
/* Lights buffer. */
StructuredBuffer<Light> lights;
/* Macrofacet illuminance cache buffer. */
RWStructuredBuffer<cache::Slot> macrofacet_illuminance_cache;
/* Visibility buffer. */
Texture2D<uint2> vbuffer;

inline float3 direct_illuminance(const float3 point, const float3 normal, const uint seed) {
    if (scene_view.light_count == 0u) return 0.0f;

    /* Fetch a light to sample */
    const uint light_index = (uint)(r1(seed) * (float)scene_view.light_count);
    const Light light = lights[light_index];

    /* Calculate the direction and distance to the light */
    const float3 light_pos = light.origin + sample_hemisphere_uniform(normalize(light.origin - point), r2(seed)) * light.source_radius;
    const float3 light_dir = normalize(light_pos - point);
    const float ndotl = max(0.0f, dot(normal, light_dir));
    if (ndotl == 0.0f) return 0.0f;
    const float light_t = distance(light_pos, point);
    if (light_t >= light.attenuation_radius) return 0.0f;

    /* Trace a ray towards the light to check for occlusions */
    const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, custom_ray(point, light_dir), light_t);
    if (t < light_t) return 0.0f;

    const float relative_dist = saturate(light_t / light.attenuation_radius);
    const float inverse_dist = 1.0f - relative_dist * relative_dist;
    const float attenuation = (inverse_dist * inverse_dist) / (1.0f + 4.0f * relative_dist);

    /* Evaluate illuminance if there is no occlusion */
    return light.luminance * ndotl * attenuation;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    int2 pixel_id = thread_id;
    int2 resolution = render_view.resolution;
    if (settings.shading_rate == 1u) {
        /* Half-rate */
        /*  [ XY ]  */
        /*  [ YX ]  */
        const int offset = (pixel_id.y + render_view.frame_index) & 0b1;
        pixel_id.x = (pixel_id.x << 1) + offset;
        resolution.x = resolution.x >> 1;
    } else if (settings.shading_rate == 2u) {
        /* Quarter-rate */
        /*  [ XY ]  */
        /*  [ ZW ]  */
        const int2 offset = int2(render_view.frame_index & 0b1, (render_view.frame_index >> 1) & 0b1);
        pixel_id = (pixel_id << 1) + offset;
        resolution = resolution >> 1;
    }

    /* Bounds check */
    if (any(pixel_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t facet_key = asuint64(vbuffer[pixel_id]);

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(facet_key)) return;

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, pixel_id);

    /* Unpack visibility data */
    const GeoData geo = unpack_geometry(unpack_visibility(facet_key), objects, ray);
    const float3 hit_point = ray.origin + ray.dir * geo.depth + geo.normal * 0.001f;

    /* Generate a seed for the current pixel */
    const uint seed = thread_id.x + thread_id.y * resolution.x + (render_view.frame_index % (resolution.x * resolution.y));

    /* Trace light occlusion */
    // const float3 ray_dir = sample_hemisphere(geo.normal, r2(seed));
    // const Ray occlusion_ray = custom_ray(hit_point + geo.normal * 0.001f, ray_dir);
    // const float t = occlusion_bvh2(tlas_nodes, object_indices, objects, occlusion_ray, 1e30f);
    // float3 illuminance = 0.0f;
    // if (t >= 1e30f) {
    //     illuminance += float3(0.3f, 0.3f, 0.3f);
    // }

    const float3 illuminance = direct_illuminance(hit_point, geo.normal, seed);

    /* Insert the sample into the illuminance cache */
    cache::insert(macrofacet_illuminance_cache, facet_key, illuminance);
}
