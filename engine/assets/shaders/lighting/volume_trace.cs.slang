/**
 * @name Irradiance Volume Update [1 thread per ray]
 * 64 Rays per probe, 16x16x16 Probes per cascade, 6 Cascades.
 * This pass computes lighting per pixel and writes the results directly into the illuminance cache.
 */
import view;
import math;
import voxel;
import bvh2;
import cache;

// /* Active render & scene view. */
// ConstantBuffer<RenderView> render_view;
// ConstantBuffer<SceneView> scene_view;
// Texture2D<float4> blue_noise;
// /* Voxel objects buffers. */
// StructuredBuffer<float4> tlas_nodes;
// StructuredBuffer<uint> object_indices;
// StructuredBuffer<VoxelObject> objects;
// /* Macrofacet cache buffer. */
// RWStructuredBuffer<cache::Slot> macrofacet_cache;
/* Intermediate output buffer. */
RWTexture2D<float3> intermediate_buffer;

/* Octahedral probe patch size in pixels. */
const static int2 PATCH_SIZE = int2(8, 8);

[shader("compute")]
[numthreads(8, 8, 1)]
void main(int2 ray_id: SV_GroupThreadID, int2 probe_id: SV_GroupID) {
    intermediate_buffer[probe_id * PATCH_SIZE + ray_id] = float3(float2(ray_id) / float2(PATCH_SIZE), 0.0f);
}
