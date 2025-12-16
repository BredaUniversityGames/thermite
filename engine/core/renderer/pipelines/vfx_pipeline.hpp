#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {
class VfxPipeline {
   public:
    VfxPipeline() {}
    ~VfxPipeline() {}

    VfxPipeline(const VfxPipeline&) = delete;
    VfxPipeline& operator=(const VfxPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);
};
}  // namespace tmt