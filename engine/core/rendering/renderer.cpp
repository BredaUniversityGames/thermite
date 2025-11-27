#include "renderer.hpp"

#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/vram_bank.hh>

#include <graphite/nodes/raster_node.hh>

#include "engine/engine.hpp"
#include "core/window.hpp"

namespace tmt {
Renderer::Renderer() : gpu(*new GPUAdapter()), render_graph(*new RenderGraph()) {}

Renderer::~Renderer() {
    delete &gpu;
    delete &render_graph;
}

void Renderer::init() {
    gpu.set_logger(color_logger, DebugLevel::Verbose);

    /* Initialize the GPU adapter */
    if (const Result r = gpu.init(true); r.is_err()) {
        printf("failed to initialize gpu adapter.\nreason: %s\n", r.unwrap_err().c_str());
        return;
    }

    /* Initialize the Render Graph */
    render_graph.set_shader_path("../engine/assets/shaders");
    render_graph.set_max_graphs_in_flight(2u); /* Double buffering */
    if (const Result r = render_graph.init(gpu); r.is_err()) {
        printf("failed to initialize render graph.\nreason: %s\n", r.unwrap_err().c_str());
        return;
    }

    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Render Target */
    const TargetDesc target {engine.window.get_window_handle()};
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        printf("failed to initialize render target.\nreason: %s\n", r.unwrap_err().c_str());
        return;
    } else
        render_target = r.unwrap();

    /* Initialize the Line Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, MAX_DEBUG_POINTS, sizeof(DebugPoint)); r.is_err()) {
        printf("failed to initialise line buffer.\nreason: %s\n", r.unwrap_err().c_str());
        return;
    } else
        point_buffer = r.unwrap();
}

static bool is_init = false;
static Buffer vertex_buffer {};

struct Vertex {
    float x;
    float y;
    float z;
};

void Renderer::update() {
    VRAMBank& bank = gpu.get_vram_bank();
    if (!is_init) {
        /* Initialise a vertex buffer */
        
        if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, 1, sizeof(Vertex) * 3); r.is_err()) {
            printf("failed to initialise constant buffer.\nreason: %s\n", r.unwrap_err().c_str());
            return;
        } else
            vertex_buffer = r.unwrap();
        std::vector<Vertex> vertices {};
        vertices.push_back({-0.5f, 0.5f, 0.0f});
        vertices.push_back({0.0f, -0.5f, 0.0f});
        vertices.push_back({0.5f, 0.5f, 0.0f});
        bank.upload_buffer(vertex_buffer, vertices.data(), 0, sizeof(Vertex) * 3);
        is_init = true;
    }
    // Temporary
    draw_line({-0.5f, 0.5f, 0.0f}, {0.0f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.0f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
    draw_line({0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});

    /* Update Line Buffer */
    if (debug_points.size() > 0) {
        bank.upload_buffer(point_buffer, debug_points.data(), 0, sizeof(DebugPoint) * debug_points.size());
    }

    render_graph.new_graph().unwrap();

    /* Test Rasterisation Pass */
    RasterNode& graphics_pass = render_graph.add_raster_pass("graphics pass", "test-vert", "test-frag")
                                    .topology(Topology::TriangleList)
                                    .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                    .attach(render_target)
                                    .raster_extent(engine.window.width, engine.window.height);
    graphics_pass.draw(vertex_buffer, 3);

    RasterNode& line_pass = render_graph.add_raster_pass("debug line pass", "debug-line-vert", "debug-line-frag")
                                .topology(Topology::LineList)
                                .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                .attribute(AttrFormat::XYZ32_SFloat)  // Color
                                .attach(render_target)
                                .raster_extent(engine.window.width, engine.window.height);
    line_pass.draw(point_buffer, (u32)debug_points.size());
    debug_points.clear();

    render_graph.end_graph().expect("failed to compile render graph.");
    render_graph.dispatch().expect("failed to dispatch render graph.");
}

void Renderer::end() {
    VRAMBank& bank = gpu.get_vram_bank();
    bank.destroy(render_target);
    bank.destroy(vertex_buffer);
    bank.destroy(point_buffer);

    /* Cleanup the VRAM bank & GPU adapter */
    render_graph.deinit().expect("failed to destroy render graph.");
    bank.deinit().expect("failed to destroy vram bank.");
    gpu.deinit().expect("failed to destroy gpu adapter.");
}

void Renderer::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color) {
    if (debug_points.size() <= MAX_DEBUG_POINTS - 2) {
        debug_points.push_back({start, color});
        debug_points.push_back({end, color});
    }
}

}  // namespace tmt
