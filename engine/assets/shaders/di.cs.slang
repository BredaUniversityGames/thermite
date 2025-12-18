import view;
import voxel;
import visibility;
import material;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer. (Input) */
RWTexture2D<uint2> vbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

struct HitInfo {
    float depth;
    float3 normal;
    Material material;
};

HitInfo evaluate_hit(const VisData vis) {
    // TODO: Use the face_index and VoxelObject transform to get the normal.
    // TODO: Use the normal and position to do a ray plane intersection, to find the depth.
    // TODO: Traverse down the 64Tree to find the material data.
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Load visibility buffer data */
    const uint2 packed = vbuffer[thread_id];

    if (null_visibility(packed)) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
    } else {
        /* Unpack visibility buffer data */
        const VisData vis = unpack_visibility(vbuffer[thread_id]);

        // ...
    }
}
