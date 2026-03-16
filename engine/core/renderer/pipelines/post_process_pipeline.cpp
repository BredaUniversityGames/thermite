#include "post_process_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

namespace tmt {

void PostProcessPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    down_sampler = bank.create_sampler("Down Sampler", Filter::Linear, AddressMode::ClampToBorder).expect("failed to create down sampler.");
}

void PostProcessPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const glm::uvec2 shading_res = render_view.gpu_view.resolution;

    for (uint32_t curr_mip = 1; curr_mip < render_view.lbuffer.meta.mips; curr_mip++) {
        const uint32_t mip_w = shading_res.x >> curr_mip;
        const uint32_t mip_h = shading_res.y >> curr_mip;

        /* clang-format off */
        render_graph.add_compute_pass("Bloom Downsample", "bloom/downsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(down_sampler)
                    .write(render_view.lbuffer.images[curr_mip])
                    .read(render_view.lbuffer.images[curr_mip - 1])
                    .push_constants(&curr_mip, 0u, sizeof(uint32_t))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }
}

void PostProcessPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(down_sampler);
}

}  // namespace tmt
