#include "debug_pipeline.hpp"

#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/raster_node.hh>

#include <glm/gtc/constants.hpp>

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

void DebugPipeline::init(GPUAdapter& gpu) {
    /* Get VRAM Bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Initialize the Line Buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Vertex | BufferUsage::TransferDst, MAX_DEBUG_POINTS, sizeof(GpuPoint)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialise line buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else
        point_buffer = r.unwrap();
}

void DebugPipeline::on_engine_update(const FrameData& time) {
    if (!timed_points.empty()) {
        for (auto& p : timed_points) p.time -= time.delta_time;

        std::erase_if(timed_points, [](const DebugPoint& p) { return p.time <= 0.0f; });
    }

    num_points = timed_points.size() + persistent_points.size();
}

void DebugPipeline::enqueue(RenderGraph& render_graph, RenderView render_view) {
    /* Get Render Image */
    const BindHandle render_image = render_view.get_render_image();

    /* Upload Point Buffer */
    if (num_points > 0) {
        std::vector<GpuPoint> packed {};

        for (int i = 0; i < timed_points.size(); i++) {
            packed.push_back(timed_points[i].point);
        }
        for (int j = 0; j < persistent_points.size(); j++) {
            packed.push_back(persistent_points[j].point);
        }

        render_graph.upload_buffer(point_buffer, packed.data(), 0, sizeof(GpuPoint) * num_points);
    }

    const glm::uvec2 render_res = render_view.gpu_view.resolution;
    RasterNode& line_pass = render_graph.add_raster_pass("debug line pass", "debug_line.vx", "debug_line.px")
                                .topology(Topology::LineList)
                                .attribute(AttrFormat::XYZ32_SFloat)  // Position
                                .attribute(AttrFormat::XYZ32_SFloat)  // Color
                                .read(render_view.render_view_buffer, ShaderStages::Vertex)
                                .attach(render_image)
                                .raster_extent(render_res.x, render_res.y);
    line_pass.draw(point_buffer, num_points);

    // Update Points
    num_points -= persistent_points.size();
    persistent_points.clear();
}

void DebugPipeline::deinit(GPUAdapter& gpu) {
    VRAMBank& bank = gpu.get_vram_bank();

    bank.destroy(point_buffer);
}

void DebugPipeline::draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color, const float time) {
    if (num_points > MAX_DEBUG_POINTS - 2) {
        Log::error(Log::Scope::RENDERER, "max amount of lines reached. Consider increasing MAX_DEBUG_POINTS, or draw less lines.");
        return;
    }

    // Timed Lines
    if (time > 0.0f) {
        timed_points.push_back({{start, color}, time});
        timed_points.push_back({{end, color}, time});
    } else {  // Persistent Lines
        persistent_points.push_back({{start, color}, time});
        persistent_points.push_back({{end, color}, time});
    }
    num_points += 2;
}

void DebugPipeline::draw_circle(const glm::vec3 center, const float radius, const glm::vec3 axis_a, const glm::vec3 axis_b, int segments, const glm::vec3 color, const float time) {
    if (segments < 4) segments = 4;

    const float step = glm::two_pi<float>() / static_cast<float>(segments);

    glm::vec3 prev = center + radius * (axis_a * cosf(0.0f) + axis_a * sinf(0.0f));

    for (int i = 1; i <= segments; ++i) {
        float angle = step * i;

        glm::vec3 curr = center + radius * (axis_a * cosf(angle) + axis_b * sinf(angle));

        draw_line(prev, curr, color, time);
        prev = curr;
    }
}

void DebugPipeline::draw_sphere(const glm::vec3 center, float radius, int rings, int segments, const glm::vec3 color, const float time) {
    if (segments < 4) segments = 4;
    if (rings < 2) rings = 2;

    const int latitude_rings = rings / 2;
    const int longitude_rings = rings - latitude_rings;

    // --- Great circles (always draw these) ---
    draw_circle(center, radius, {1, 0, 0}, {0, 1, 0}, segments, color, time);
    draw_circle(center, radius, {1, 0, 0}, {0, 0, 1}, segments, color, time);
    draw_circle(center, radius, {0, 1, 0}, {0, 0, 1}, segments, color, time);

    // --- Latitude rings (horizontal) ---
    for (int i = 1; i <= latitude_rings; ++i) {
        float t = float(i) / float(latitude_rings + 1);
        float y = glm::mix(-radius, radius, t);

        float ring_radius = sqrtf(radius * radius - y * y);

        draw_circle(center + glm::vec3(0, y, 0), ring_radius, {1, 0, 0}, {0, 0, 1}, segments, color, time);
    }

    // --- Longitude rings (vertical) ---
    for (int i = 0; i < longitude_rings; ++i) {
        float angle = glm::two_pi<float>() * float(i) / float(longitude_rings);

        glm::vec3 axisA = {cosf(angle), 0.0f, sinf(angle)};

        glm::vec3 axisB = {0.0f, 1.0f, 0.0f};

        draw_circle(center, radius, axisA, axisB, segments, color, time);
    }
}

}  // namespace tmt