import view;
import voxel;
import visibility;
import material;
import math;
import bvh2;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Visibility buffer. (Input) */
RWTexture2D<uint2> vbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

static const float3 LIGHT_COLOR = float3(1.0f, 0.1f, 0.1f);
static const float3 LIGHT_POS = float3(2.0f, 12.0f, 4.0f);
static const float LIGHT_STRENGTH = 256.0f;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Load visibility buffer data */
    const uint2 packed = vbuffer[thread_id];

    /* Exit early if the visibility for this pixel is null */
    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
        return;
    }

    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Unpack visibility buffer data */
    const VisData vis = unpack_visibility(vbuffer[thread_id]);
    const GeoData geo = unpack_geometry(vis, object_data, ray);

    /* Find the hit position in world-space */
    const float3 hit_pos = ray.origin + ray.dir * geo.depth + geo.normal * (UNITS_PER_VOXEL * 0.01f);
    const float3 dir_to_light = normalize(LIGHT_POS - hit_pos);
    const float light_distance = distance(hit_pos, LIGHT_POS);

    /* Trace the scene BVH for visibility */
    const Ray vis_ray = custom_ray(hit_pos, dir_to_light);
    const float hit_t = occlusion_bvh2(bvh_nodes, object_indices, object_data, vis_ray, light_distance);

    float3 illuminance = 0.0f;
    if (hit_t >= light_distance) {
        illuminance = max(0.0f, dot(geo.normal, dir_to_light)) * LIGHT_STRENGTH * LIGHT_COLOR * (1.0f / (light_distance * light_distance));
    }

    render_target[thread_id] = float4(geo.material.albedo * illuminance, 1.0f);
}
