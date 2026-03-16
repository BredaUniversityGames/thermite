#include "post_process_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

namespace tmt {

void PostProcessPipeline::init(GPUAdapter&) {}

void PostProcessPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const glm::uvec2 shading_res = render_view.gpu_view.resolution;

    /* clang-format off */
    for(uint32_t curr_mip = 1; curr_mip < render_view.lbuffer.meta.mips; curr_mip++) {
    render_graph.add_compute_pass("Bloom Downsample", "bloom/downsample.cs")
                .write(render_view.lbuffer.images[0])
                .push_constants(&curr_mip, 0, sizeof(uint32_t))
                .group_size(16, 8)
                .work_size(shading_res.x, shading_res.y);
    }
    /* clang-format on */
}

void PostProcessPipeline::deinit(GPUAdapter&) {}

}  // namespace tmt
