struct Counters {
    uint32_t alive_count;
    uint32_t alive_count_after_sim;
    uint32_t dead_count;
    uint32_t draw_count;
};

struct DrawhArgs {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t first_instance;
};

RWStructuredBuffer<Counters> counters;
RWStructuredBuffer<DrawhArgs> draw_args;

[shader("compute")]
[numthreads(1, 1, 1)]
void main(uint3 thread_id: SV_DispatchThreadID) {
    uint32_t particle_count = counters[0].draw_count;

    DrawhArgs args;
    args.vertex_count = 6;
    args.instance_count = particle_count;
    args.first_vertex = 0;
    args.first_instance = 0;

    draw_args[0] = args;
}