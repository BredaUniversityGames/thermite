#include "vfx_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {
void VfxPipeline::init(GPUAdapter& gpu) {
    auto& bank = engine.renderer.vram_bank();

    /* Initialize the Billboard Vertex Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, 6, sizeof(glm::vec2)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise billboard vertex buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        billboard_vertices = r.unwrap();

    /* Initialize the Billboards Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, 1, sizeof(Billboard)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise billboards buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        billboards = r.unwrap();

    glm::vec2 vertices[6] = {
        {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f},

        {-0.5f, -0.5f}, {0.5f, 0.5f},  {-0.5f, 0.5f},
    };
    bank.upload_buffer(billboard_vertices, &vertices, 0, sizeof(vertices));
}

static Billboard billboard {};

void VfxPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    billboard.color = {1, 0, 0, 1};
    billboard.pos = {0, 4, 0};
    billboard.size = 1.0f;

    render_graph.upload_buffer(billboards, &billboard, 0, sizeof(billboard));

    const BindHandle render_image = render_view.get_render_image();

    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("billboard pass", "billboard.vx", "billboard.px")
                                .topology(Topology::TriangleList)
                                .attribute(AttrFormat::XY32_SFloat)  // Position
                                .read(render_view.gpu_view_buffer, ShaderStages::Vertex)
                                .read(billboards, ShaderStages::Vertex)
                                .load_op(LoadOp::Load)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw(billboard_vertices, 6, 0, 1, 0);
}

void VfxPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(billboards);
    bank.destroy(billboard_vertices);
}

}  // namespace tmt
