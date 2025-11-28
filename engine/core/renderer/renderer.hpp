#pragma once

#include <vector>

#include <graphite/imgui.hh>
#include <graphite/resources/handle.hh>

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct DebugPoint {
    glm::vec3 start {0.0f, 0.0f, 0.0f};
    glm::vec3 color {1.0f, 0.0f, 0.0f};
};

constexpr uint32_t MAX_DEBUG_POINTS = 32;

class Renderer {
    // Debug Rendering
    Buffer point_buffer {};
    std::vector<DebugPoint> debug_points {};

   public:
    GPUAdapter& gpu;
    RenderGraph& render_graph;
    RenderTarget render_target {};
    ImGUI imgui {};

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f});
};
}  // namespace tmt