#pragma once

#include <graphite/resources/handle.hh>

#include "core/renderer/render_view.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

class PostProcessPipeline {
   public:
    PostProcessPipeline() {}
    ~PostProcessPipeline() {}

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

    /* Partial Luminance texture for auto-exposure (256x256) */
    Texture lum_partial_texture {};
    Image lum_partial_image {};
    /* Tiny Luminance texture for auto-exposure (16x16) */
    Texture lum_tiny_texture {};
    Image lum_tiny_image {};

    Sampler down_sampler {};
    Sampler up_sampler {};
};

}  // namespace tmt