struct ParticleEmitter {
    float3 pos;
    float lifetime;

    float3 dir;
    float cone_angle;

    float min_speed;
    float max_speed;

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

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float rand01(uint seed) {
    return (hash(seed) & 0x00FFFFFF) / 16777216.0;
}

float3 random_in_cone(float3 dir, float angle, uint seed) {
    float u = rand01(seed);
    float v = rand01(seed + 1);

    float cos_theta = lerp(cos(angle), 1.0f, u);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    float phi = v * 6.2831853f;  // 2pi

    // Direction in cone space (around +Z)
    float3 local_dir = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    // Build orthonormal basis for emitter direction
    float3 w = normalize(dir);
    float3 up = abs(w.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 u_axis = normalize(cross(up, w));
    float3 v_axis = cross(w, u_axis);

    // Rotate local direction into world space
    return normalize(u_axis * local_dir.x + v_axis * local_dir.y + w * local_dir.z);
}

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    if (thread_id.x >= emitter.spawn_count)
        return;

    if (emitter.spawn_count >= counters[0].dead_count)
        return;

    Particle p;
    p.pos = emitter.pos;
    p.age = 0.0f;
    p.lifetime = emitter.lifetime;

    uint seed = thread_id.x + counters[0].alive_count * 1337;
    float3 dir = random_in_cone(emitter.dir, emitter.cone_angle, seed);
    float speed = lerp(emitter.min_speed, emitter.max_speed, rand01(seed + 2));

    p.velocity = dir * speed;
    p.lifetime = emitter.lifetime;

    uint32_t count_dead;
    InterlockedAdd<uint32_t>(counters[0].dead_count, -1, count_dead);

    uint32_t new_particle_index = dead_list[count_dead - 1];

    particles[new_particle_index] = p;

    uint32_t alive_count;
    InterlockedAdd<uint32_t>(counters[0].alive_count_after_sim, 1, alive_count);
    alive_list[alive_count] = new_particle_index;
}
