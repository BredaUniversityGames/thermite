import view;
import voxel;
import bvh2;
import math;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Object buffers. */
StructuredBuffer<float4> bvh_nodes;
StructuredBuffer<uint> object_indices;
StructuredBuffer<VoxelObject> object_data;
/* Visibility buffer. (Output) */
RWTexture2D<uint2> vbuffer;

/* Pack voxel visibility into two 32-bit unsigned integers. */
[ForceInline] uint2 pack_visibility(Hit hit) {
    if (hit.t == BIG_F32) return uint::maxValue;
    const uint x = hit.voxel_index;
    const uint y = (hit.object_index & 0x0000FFFFu) | (hit.face_index << 16);
    return uint2(x, y);
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Generate a ray for our render view */
    const Ray ray = pinhole_ray(render_view, thread_id);

    /* Trace the scene BVH */
    const Hit hit = trace_bvh2<false>(bvh_nodes, object_indices, object_data, ray);

    /* Write packed visibility info into the visibility buffer */
    vbuffer[thread_id] = pack_visibility(hit);
}
