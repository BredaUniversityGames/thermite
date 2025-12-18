#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_PARTICLES = 100;

//struct GpuParticle {
//    glm::vec4 color {};
//    glm::vec3 pos {};
//};

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

    Buffer billboard_vertices {};
    Buffer billboards {};
};
}  // namespace tmt