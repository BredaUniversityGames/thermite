#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_PARTICLES = 100;

struct GpuParticle {
    glm::vec4 color {};
    glm::vec3 pos {};
};

struct Counters {
    uint32_t alive_count {};
    uint32_t alive_count_after_sim {};
    uint32_t dead_count {};
    uint32_t draw_count {}; // Amount of particles to be drawed
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

    Buffer counter_buffer {};
    Buffer alive_list {};
    Buffer alive_list_new {};
    Buffer dead_list {};
    Buffer particle_buffer {};
    Buffer dispatch_args_buffer {};
    Buffer instanced_draw_args_buffer {};
    Buffer emitter_buffer {};

    Buffer billboard_vertices {};
    Buffer billboards {};
};
}  // namespace tmt