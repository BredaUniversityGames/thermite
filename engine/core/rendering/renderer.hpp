#pragma once

#include <vector>

#include <graphite/vram_bank.hh>

class GPUAdapter;
class RenderGraph;

namespace tmt {

struct DebugLine {
    glm::vec3 start {0.0f, 0.0f, 0.0f};
    glm::vec3 end {0.0f, 0.0f, 0.0f};
    glm::vec3 color {1.0f, 0.0f, 0.0f};
};

constexpr uint32_t MAX_DEBUG_LINES = 512;

class Renderer {
    std::vector<DebugLine> debug_lines {};

   public:
    GPUAdapter& gpu;
    RenderGraph& render_graph;
    RenderTarget render_target {};

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