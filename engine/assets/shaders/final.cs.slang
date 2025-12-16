import view;
import math;
import voxel;
import bvh2;
import material;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Render target. */
// Texture2D<float4> render_target;
/* Render target. */
RWTexture2D<float4> render_target;

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    
}
