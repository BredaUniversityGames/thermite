#include "post_process_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/renderer/renderer.hpp"
#include "tools/player_data.hpp"

namespace tmt {

void PostProcessPipeline::init(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    lum_partial_texture = bank.create_texture("Partial Lum Buffer Texture", TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA16Sfloat, {256u, 256u}).expect("failed to initialize partial lum buffer texture");
    lum_partial_image = bank.create_image("Partial Lum Buffer Image", lum_partial_texture).expect("failed to initialize partial lum buffer image.");
    lum_tiny_texture = bank.create_texture("Tiny Lum Buffer Texture", TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA16Sfloat, {16u, 16u}).expect("failed to initialize tiny lum buffer texture");
    lum_tiny_image = bank.create_image("Tiny Lum Buffer Image", lum_tiny_texture).expect("failed to initialize tiny lum buffer image.");

    down_sampler = bank.create_sampler("Down Sampler", Filter::Linear, AddressMode::ClampToBorder).expect("failed to create down sampler.");
    up_sampler = bank.create_sampler("Up Sampler", Filter::Linear, AddressMode::ClampToEdge).expect("failed to create up sampler.");
}

void PostProcessPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    const glm::uvec2 shading_res = render_view.gpu_view.resolution;

    RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

    { /* Auto-exposure */
        /* clang-format off */
        render_graph.add_compute_pass("Luminance Gather (Auto-exposure)", "lighting/lum_gather.cs")
            .read(render_view.render_view_buffer)
            .read(render_view.lbuffer.image)
            .write(lum_partial_image)
            .group_size(8, 8)
            .work_size(256, 256);

        render_graph.add_compute_pass("Luminance Average (Auto-exposure)", "lighting/lum_average.cs")
            .read(lum_partial_image)
            .write(lum_tiny_image)
            .read(down_sampler)
            .group_size(8, 8)
            .work_size(128, 128);
        /* clang-format on */
    }

    /* Threshold the Luminance buffer */
    {
        /* clang-format off */
        render_graph.add_compute_pass("Luminance Threshold", "luminance_threshold.cs")
                    .read(render_view.lbuffer.image)
                    .write(render_view.tbuffer.images[0])
                    .push_constants(&settings.bloom_threshold, 0u, sizeof(float))
                    .group_size(16, 8)
                    .work_size(shading_res.x, shading_res.y);
        /* clang-format on */
    }

    for (uint32_t curr_mip = 1; curr_mip < render_view.tbuffer.meta.mips; curr_mip++) {
        const uint32_t mip_w = shading_res.x >> curr_mip;
        const uint32_t mip_h = shading_res.y >> curr_mip;

        /* clang-format off */
        render_graph.add_compute_pass("Bloom Downsample", "bloom/downsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(down_sampler)
                    .write(render_view.tbuffer.images[curr_mip])
                    .read(render_view.tbuffer.images[curr_mip - 1])
                    .push_constants(&curr_mip, 0u, sizeof(uint32_t))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }

    for (int32_t curr_mip = (int32_t)render_view.tbuffer.meta.mips - 2; curr_mip >= 0; curr_mip--) {
        const uint32_t mip_w = shading_res.x >> curr_mip;
        const uint32_t mip_h = shading_res.y >> curr_mip;

        struct UpsampleConstants {
            uint32_t curr_mip_level;
            float bloom_radius;
        } upsample_constants;

        upsample_constants.curr_mip_level = curr_mip;
        upsample_constants.bloom_radius = settings.bloom_radius;

        /* clang-format off */
        render_graph.add_compute_pass("Bloom Upsample", "bloom/upsample.cs")
                    .read(render_view.render_view_buffer)
                    .read(up_sampler)
                    .write(render_view.tbuffer.images[curr_mip])
                    .read(render_view.tbuffer.images[curr_mip + 1])
                    .push_constants(&upsample_constants, 0u, sizeof(UpsampleConstants))
                    .group_size(16, 8)
                    .work_size(mip_w, mip_h);
        /* clang-format on */
    }

    /* Bloom Addition, Color Grading and Tonemapping */
    /* clang-format off */
        render_graph.add_compute_pass("Color Grading and Tonemapping", "cg_tonemap.cs")
                    .read(render_view.lbuffer.image)
                    .read(render_view.tbuffer.images[0])
                    .write(render_view.get_render_image())
                    .group_size(16, 8)
                    .work_size(render_view.gpu_view.resolution.x, render_view.gpu_view.resolution.y);
    /* clang-format on */
}

void PostProcessPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(lum_partial_image);
    bank.destroy(lum_partial_texture);
    bank.destroy(lum_tiny_image);
    bank.destroy(lum_tiny_texture);
    bank.destroy(down_sampler);
    bank.destroy(up_sampler);
}

}  // namespace tmt
