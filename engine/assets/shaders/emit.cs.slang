struct ParticleEmitter {
    float4 start_color;
    float4 end_color;

    float3 pos;
    float lifetime;

    float3 velocity_min;
    float velocity_max;

    uint32_t spawn_count;
};

struct Particle {
    float3 pos;
    float age;

    float3 velocity;
    float lifetime;
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

float hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return (float)x / 4294967295.0;
}

float3 random_dir(uint seed)
{
    float z = hash(seed) * 2.0f - 1.0f;
    float a = hash(seed + 1) * 6.2831853f;
    float r = sqrt(1.0f - z * z);
    return float3(r * cos(a), z, r * sin(a));
}

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    if (thread_id.x >= emitter.spawn_count)
        return;

    Particle p;
    p.pos = emitter.pos;
    p.age = 0.0f;
    p.lifetime = emitter.lifetime;

    float3 dir = normalize(random_dir(thread_id.x));
    float speed = lerp(
        length(emitter.velocity_min),
        length(emitter.velocity_max),
        hash(thread_id.x + 7)
    );

    p.velocity = dir * speed;

    uint32_t count_dead;
    InterlockedAdd<uint32_t>(counters[0].dead_count, -1, count_dead);

    uint32_t new_particle_index = dead_list[count_dead - 1];

    particles[new_particle_index] = p;

    uint32_t alive_count;
    InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, alive_count);
    alive_list[alive_count] = new_particle_index;
}