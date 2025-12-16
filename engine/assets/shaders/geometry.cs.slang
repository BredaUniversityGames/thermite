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
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    const uint x = hit.voxel_index;
    const uint y = (hit.face_index << 16) | (hit.object_index & 0xFFFFu);
    return uint2(x, y);
}

/* Unpack voxel visibility. */
[ForceInline] Hit unpack_visibility(uint2 packed) {
    /* x = voxel_index (32) */
    /* y = face_index (3), object_index (16) */
    Hit hit;
    hit.voxel_index = packed.x;
    hit.object_index = packed.y & 0xFFFFu;
    hit.face_index = packed.y >> 16;
    return hit;
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
