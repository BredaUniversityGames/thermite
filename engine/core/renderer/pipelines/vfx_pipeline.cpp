#include "vfx_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>
#include <graphite/nodes/compute_node.hh>

#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"
#include "core/components/emitter.hpp"

namespace tmt {
void VfxPipeline::init(GPUAdapter& gpu) {
    auto& bank = engine.renderer.vram_bank();

    /* Initialize the Counters Buffer */
    {
        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, 1, sizeof(Counters)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the counters buffer.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            counter_buffer = r.unwrap();

        Counters counters = {};
        counters.alive_count = 0;
        counters.alive_count_after_sim = 0;
        counters.dead_count = MAX_PARTICLES;
        counters.draw_count = 0;

        bank.upload_buffer(counter_buffer, &counters, 0, sizeof(Counters));
    }

    /* Initialize the Alive Lists */
    {
        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the alive list.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            alive_list = r.unwrap();

        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the alive list new.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            alive_list_new = r.unwrap();

        uint32_t arr[MAX_PARTICLES] {};
        size_t size = sizeof(uint32_t) * MAX_PARTICLES;
        memset(&arr, 0, size);

        bank.upload_buffer(alive_list, &arr, 0, size);
        bank.upload_buffer(alive_list_new, &arr, 0, size);
    }

    /* Initialize the Dead List */
    {
        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(uint32_t)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the dead list.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            dead_list = r.unwrap();

        uint32_t arr[MAX_PARTICLES] {};
        for (size_t i = 0; i < MAX_PARTICLES; i++) {
            arr[i] = i;
        }
        bank.upload_buffer(dead_list, &arr, 0, sizeof(uint32_t) * MAX_PARTICLES);
    }

    /* Initialize the Particle Buffer */
    {
        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::TransferDst, MAX_PARTICLES, sizeof(GpuParticle)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the particle buffer.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            particle_buffer = r.unwrap();
    }

    /* Initialise the Indirect Buffers */
    {
        struct IndirectDispatchArgs {
            uint32_t x;
            uint32_t y;
            uint32_t z;
        };

        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::Indirect, sizeof(IndirectDispatchArgs)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the indirect dispatch args buffer.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            dispatch_args_buffer = r.unwrap();

        struct IndirectDrawArgs {
            uint32_t vertex_count;
            uint32_t instance_count;
            uint32_t first_vertex;
            uint32_t first_instance;
        };

        if (const Result r = bank.create_buffer(BufferUsage::Storage | BufferUsage::Indirect, sizeof(IndirectDrawArgs)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the indirect draw args buffer.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            instanced_draw_args_buffer = r.unwrap();
    }

    /* Initialise Emitter Buffer */
    {
        if (const Result r = bank.create_buffer(BufferUsage::Constant | BufferUsage::TransferDst, sizeof(ParticleEmitter)); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to initialise the particle emitter buffer.\nreason: {}", r.unwrap_err().c_str());
            return;
        } else
            emitter_buffer = r.unwrap();
    }

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

    VRAMBank& bank = engine.renderer.vram_bank();

    render_graph.upload_buffer(billboards, &billboard, 0, sizeof(billboard));

    const BindHandle render_image = render_view.get_render_image();
    /* clang-format off */

    const entt::basic_group group = engine.ecs.get_registry().group<const ParticleEmitter>(entt::get<Transform>);
    for (auto&& [entity, emitter, transform] : group.each()) {
        render_graph.upload_buffer(emitter_buffer, &emitter, 0, sizeof(ParticleEmitter));

        render_graph.add_compute_pass("Emit Particles", "emit.cs")
                    .read(emitter_buffer)
                    .write(counter_buffer)
                    .write(alive_list)
                    .write(alive_list_new)
                    .write(dead_list)
                    .write(particle_buffer)
                    .group_size(64, 1, 1)
                    .work_size(emitter.particles, 1, 1);
    }

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

    /* clang-format on */
}

void VfxPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(instanced_draw_args_buffer);
    bank.destroy(dispatch_args_buffer);
    bank.destroy(particle_buffer);
    bank.destroy(dead_list);
    bank.destroy(alive_list_new);
    bank.destroy(alive_list);
    bank.destroy(counter_buffer);
    bank.destroy(emitter_buffer);

    bank.destroy(billboards);
    bank.destroy(billboard_vertices);
}

}  // namespace tmt
