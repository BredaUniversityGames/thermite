#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_PARTICLES = 100'000;
constexpr uint32_t MAX_EMITTERS = 128;

struct GpuQuad {
    glm::vec2 pos {};
    glm::vec2 uvs {};
};

struct GpuEmitter {
    glm::vec3 pos {};
    float lifetime {};

    glm::vec3 dir {};
    float cone_angle {};

    float min_speed {};
    float max_speed {};
    uint32_t spawn_count {};
    uint32_t tex_index {};

    float size {};
    float end_size {};
    float opacity {};
    float end_opacity {};
};

struct GpuParticle {
    glm::vec3 pos {};
    float age {};

    glm::vec3 velocity {};
    float lifetime {};

    float size {};
    float start_size {};
    float end_size {};
    float opacity {};

    float start_opacity {};
    float end_opacity {};
    uint32_t tex_index {};
    float pad {};
};

struct Counters {
    uint32_t alive_count {};
    uint32_t alive_count_after_sim {};
    uint32_t dead_count {};
    uint32_t draw_count {};  // Amount of particles to be drawn
};

struct Billboard {
    glm::vec4 color {};
    glm::vec3 pos {};
    float size {};
};

class VfxPipeline {
   public:
    VfxPipeline() {}
    ~VfxPipeline() {}

    VfxPipeline(const VfxPipeline&) = delete;
    VfxPipeline& operator=(const VfxPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    /* Buffers */
    Buffer counter_buffer {};
    Buffer alive_list {};
    Buffer alive_list_new {};
    Buffer dead_list {};
    Buffer particle_buffer {};
    Buffer dispatch_args_buffer {};
    Buffer instanced_draw_args_buffer {};
    Buffer emitter_buffer {};
    Buffer billboard_vertices {};

    Sampler linear_sampler {};
};

}  // namespace tmt