import view;
import voxel;
import visibility;
import math;
import colorspace;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<VoxelObject> objects;
/* Visibility buffer. (Input) */
Texture2D<uint2> vbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

/* Size of the froxel grid in froxels. */
static const int3 FROXEL_GRID_SIZE = int3(160 / 4, 90 / 4, 64 / 4);

/* Get the center position of a froxel in world-space. */
public [ForceInline] float3 froxel_to_world(const int3 froxel, const float4x4 clip_to_world, const float near_z, const float far_z) {
    /* Calculate the normalized device z coordinate (exponential distribution) */
    const float z_center = near_z * pow(far_z / near_z, (float(froxel.z) + 0.5f) / FROXEL_GRID_SIZE.z);
    const float z_ndc = (far_z * near_z / z_center - far_z) / (near_z - far_z);

    /* Calculate the normalize device x and y coordinates */
    const float x_ndc = (float(froxel.x) + 0.5f) / (FROXEL_GRID_SIZE.x * 0.5f) - 1.0f;
    const float y_ndc = (float(froxel.y) + 0.5f) / (FROXEL_GRID_SIZE.y * 0.5f) - 1.0f;

    /* Transform from clip-space to world-space */
    const float4 view = mul(clip_to_world, float4(x_ndc, y_ndc, z_ndc, 1.0f));
    return view.xyz / view.w;
}

/* Get the UVW of the froxel volume at a given world-space position. */
public [ForceInline] float3 froxel_uvw(const float3 pos, const float4x4 world_to_clip, const float near_z, const float far_z) {
    /* Transform world-space to clip-space, then NDC */
    const float4 clip = mul(world_to_clip, float4(pos, 1.0f));
    const float3 ndc = clip.xyz / clip.w;

    /* Invert the NDC x,y mapping */
    const float u = ndc.x * 0.5f + 0.5f;
    const float v = ndc.y * 0.5f + 0.5f;

    /* Recover linear depth from NDC z, then invert the exponential distribution */
    const float z_linear = far_z * near_z / (ndc.z * (near_z - far_z) + far_z);
    const float w = log(z_linear / near_z) / log(far_z / near_z);
    return float3(u, v, w);
}


[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Bounds check */ 
    if (any(thread_id >= render_view.resolution)) return;

    /* Load visibility buffer data */
    const uint64_t packed = asuint64(vbuffer[thread_id]);

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Generate a ray for our render view */
        const Ray ray = pinhole_ray(render_view, thread_id);

        /* Unpack visibility buffer data */
        const VisData vis = unpack_visibility(packed);
        const GeoData geo = unpack_geometry(vis, objects, ray);

        /* Calculate some fake shading for the view */
        const float fake_shading = clamp(dot(-ray.dir, geo.normal), 0.2f, 0.8f);
        render_target[thread_id] = float4(cs::delinearize(cs::acescg_to_r709(geo.material.albedo() * fake_shading)), 1.0f);
        // render_target[thread_id] = float4(froxel_uvw(ray.origin + ray.dir * geo.depth, render_view.world_to_clip, 0.1f, 320.0f), 1.0f);
    }
}
