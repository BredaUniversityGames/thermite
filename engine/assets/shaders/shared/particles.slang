module particles;

import math;

public struct BillboardVertexOutput {
    public float4 pos : SV_Position;
    public float2 uv : TEXCOORD0;
    public uint32_t tex_index : TEXINDEX0;
    public float opacity : OPACITY0;
};

public struct ParticleEmitter {
    public float3 pos;
    public float lifetime;

    public float3 dir;
    public float cone_angle;

    public Range speed;
    public uint32_t spawn_count;
    public uint32_t tex_index;

    public Range start_size;
    public Range end_size;

    public Range start_opacity;
    public Range end_opacity;

    public Range rotation;
};

public struct Particle {
    public float3 pos;
    public float age;

    public float3 velocity;
    public float lifetime;

    public float size;
    public float start_size;
    public float end_size;
    public float opacity;

    public float start_opacity;
    public float end_opacity;
    public float rot_speed;
    public float rotation;

    public uint32_t tex_index;
};

public struct Counters {
    public uint32_t alive_count;
    public uint32_t alive_count_after_sim;
    public uint32_t dead_count;
    public uint32_t draw_count;
};

public struct DrawArgs {
    public uint32_t vertex_count;
    public uint32_t instance_count;
    public uint32_t first_vertex;
    public uint32_t first_instance;
};

public struct DispatchArgs {
    public uint32_t x;
    public uint32_t y;
    public uint32_t z;
};