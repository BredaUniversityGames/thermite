#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

/* Direct Illumination (DI) pipeline. */
class DiPipeline {
   private:

   public:
    DiPipeline() {}
    ~DiPipeline() {}

    DiPipeline(const DiPipeline&) = delete;
    DiPipeline& operator=(const DiPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);
};

}  // namespace tmt
