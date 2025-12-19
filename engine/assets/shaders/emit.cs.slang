struct ParticleEmitter {
    float4 start_color;
    float4 end_color;

    float3 start_pos;
    float lifetime;

    float3 end_pos;
    uint32_t particles;
};

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

ConstantBuffer<ParticleEmitter> emitter;

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<uint32_t> alive_list;
RWStructuredBuffer<uint32_t> alive_list_new;
RWStructuredBuffer<uint32_t> dead_list;
RWStructuredBuffer<Particle> particles;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    if (thread_id.x >= emitter.particles)
        return;

    Particle particle;
    particle.dir = normalize(emitter.end_pos - emitter.start_pos);
    particle.life = emitter.lifetime;
    particle.pos = emitter.start_pos;
    particle.speed = 0.1f;

    uint32_t count_dead;
    InterlockedAdd<uint32_t>(counters[0].dead_count, -1, count_dead);

    uint32_t new_particle_index = dead_list[count_dead - 1];

    particles[new_particle_index] = particle;

    uint32_t alive_count;
    InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, alive_count);
    alive_list[alive_count] = new_particle_index;
}