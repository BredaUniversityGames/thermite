import view;
import voxel;

/* Active render view. */
ConstantBuffer<RenderView> render_view;
/* Visibility buffer. (Input) */
RWTexture2D<uint2> vbuffer;
/* Render target. (Output) */
RWTexture2D<float4> render_target;

/* 3D PCG hash function. */
[ForceInline] uint3 pcg3d(const uint3 seed) {
    const uint3 state = seed * 747796405u + 2891336453u;
    const uint3 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

/* Generate a random color based on a seed. */
[ForceInline] float3 color(const uint seed) {
    const uint3 r = pcg3d(uint3(seed, seed << 12, seed << 24));
    return 0.25 + 0.75 * ((float3)r * (1.0 / float(0xffffffffu)));
}

/* Unpack voxel visibility. */
[ForceInline] Hit unpack_visibility(uint2 packed) {
    Hit hit;
    hit.voxel_index = packed.x;
    hit.object_index = packed.y & 0x0000FFFFu;
    hit.face_index = packed.y >> 16;
    return hit;
}

[shader("compute")]
[numthreads(16, 8, 1)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Generate a ray for our render view */
    // const Ray ray = pinhole_ray(render_view, thread_id);

    /* Unpack visibility buffer data */
    const uint2 packed = vbuffer[thread_id];

    if (packed.x == uint::maxValue) {
        render_target[thread_id] = float4(0.075f, 0.075f, 0.075f, 1.0f);
        return;
    }

    /* Render the visibility data as hash */
    const Hit hit = unpack_visibility(packed);
    render_target[thread_id] = float4(color(voxel_hash(hit.voxel_index, hit.object_index, hit.face_index)), 1.0f);
}
