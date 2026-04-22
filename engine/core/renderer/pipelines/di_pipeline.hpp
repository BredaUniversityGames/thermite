#pragma once

#include <graphite/resources/handle.hh>

class RenderGraph;
class GPUAdapter;

namespace tmt {

struct RenderView;
struct SceneView;

constexpr uint32_t CACHE_SIZE = 10000000u;

/* Direct Illumination (DI) pipeline. */
class DiPipeline {
   public:
    DiPipeline() {}
    ~DiPipeline() {}

    DiPipeline(const DiPipeline&) = delete;
    DiPipeline& operator=(const DiPipeline&) = delete;

    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view, SceneView& scene_view);
    void deinit(GPUAdapter& gpu);

    /* Intermediate probe texture */
    Texture probe_lum_texture {};
    Image probe_lum_image {};
    Texture probe_dir_texture {};
    Image probe_dir_image {};

    bool cache_init = false;
};

}  // namespace tmt
