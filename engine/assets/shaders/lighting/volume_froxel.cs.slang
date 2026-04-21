/**
 * @name Irradiance Volume Froxels [1 thread per froxel]
 * ...
 */
import view;
import math;
import volume;

/* Probe buffer. */
Texture2D<float3> probe_buffer;
/* Linear sampler. */
SamplerState linear_sampler;
/* Froxel buffer. */
RWTexture3D<float3> froxel_buffer;

[shader("compute")]
[numthreads(4, 4, 4)]
void main(int2 thread_id: SV_DispatchThreadID) {
    /* Center position of the cascade hierarchy */
    const float3 cascade_center = 0.0f;

    /* 1. Find world-space position of the froxel cell */

    /* 2. Find probe UV in probe buffer of world-space position */

    /* 3. Integrate probe irra */
}
