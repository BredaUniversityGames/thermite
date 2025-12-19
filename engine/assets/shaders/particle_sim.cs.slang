struct Particle {
    float3 pos;
    float speed;

    float3 dir;
    float life;
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

RWStructuredBuffer<float4> vertex_positions;

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    uint32_t alive_counter = counters[0].alive_count;
    if (thread_id.x >= alive_counter)
        return;

    uint32_t particle_index = alive_list[thread_id.x];
    Particle particle = particles[particle_index];
    uint32_t v0 = particle_index * 6;

    if (particle.life > 0) {
        particle.life -= utils.dt;

        particle.pos += particle.dir * particle.speed * utils.dt;

        particles[particle_index] = particle;

        uint32_t new_alive_index;
        InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, new_alive_index);
        alive_list_new[new_alive_index] = particle_index;

        InterlockedAdd(counters[0].draw_count, 1);

        for (uint32_t vertex_id = 0; vertex_id < 6; ++vertex_id) {
            vertex_positions[v0 + vertex_id] = float4(particle.pos, 0.0f);
        }
    }
    else {
        uint32_t dead_index;
        InterlockedAdd<uint32_t>(counters[0].dead_count, 1, dead_index);
        dead_list[dead_index] = particle_index;
    }
}
