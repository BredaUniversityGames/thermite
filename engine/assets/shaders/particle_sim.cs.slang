struct Particle {
    float3 pos;
    float age;

    float3 velocity;
    float lifetime;

    uint32_t tex_index;
};

struct Counters {
    uint32_t alive_count;
    uint32_t alive_count_after_sim;
    uint32_t dead_count;
    uint32_t draw_count;
};

static const float3 BILLBOARD[] = {
    float3(-1, -1, 0), // BL
    float3(1, -1, 0),  // BR
    float3(-1, 1, 0),  // TL
    float3(1, -1, 0),  // BR
    float3(1, 1, 0),   // TR
    float3(-1, 1, 0)   // TL
};

struct Utils {
    float dt;
};

ConstantBuffer<Utils> utils;

RWStructuredBuffer<Particle> particles;
RWStructuredBuffer<uint32_t> alive_list;
RWStructuredBuffer<uint32_t> alive_list_new;
RWStructuredBuffer<uint32_t> dead_list;
RWStructuredBuffer<Counters> counters;

//RWStructuredBuffer<float4> vertex_positions;

static const float3 GRAVITY = float3(0, -9.81f, 0);

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    uint32_t alive_counter = counters[0].alive_count;
    if (thread_id.x >= alive_counter)
        return;

    uint32_t particle_index = alive_list[thread_id.x];
    Particle p = particles[particle_index];
    uint32_t v0 = particle_index * 6;

    p.age += utils.dt;

    if (p.age < p.lifetime) {
        // p.velocity += GRAVITY * utils.dt;
        p.pos += p.velocity * utils.dt;

        particles[particle_index] = p;

        uint32_t new_alive_index;
        InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, new_alive_index);
        alive_list_new[new_alive_index] = particle_index;

        InterlockedAdd(counters[0].draw_count, 1);
    }
    else {
        uint32_t dead_index;
        InterlockedAdd<uint32_t>(counters[0].dead_count, 1, dead_index);
        dead_list[dead_index] = particle_index;
    }
}
