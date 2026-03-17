#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/core/renderer/scene_view.hpp"
#include "engine/events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

constexpr uint32_t MAX_UI_IMAGES = 256;
constexpr uint32_t MAX_UI_GLYPHS = 4096;

struct ImageVertex {
    glm::vec3 pos {};
    glm::vec2 uvs {};
};

struct GpuImage {
    glm::vec2 pos {};
    glm::vec2 extent {};
    glm::vec4 color {};
    glm::vec3 angles {};
    uint32_t image_index {};
    glm::vec2 pivot {};
    float d0 = {}, d1 = {}; /* dummies */
};

/*
 * GPU data for a single text glyph.
 * Must match the GlyphInstance struct in ui_text.slang
 */
struct GpuGlyph {
    glm::vec4 pos_size {};      /* .xy = screen position, .zw = glyph size */
    glm::vec4 uv_rect {};       /* .xy = min UV, .zw = max UV */
    glm::vec4 color {};         /* text color RGBA */
    glm::vec4 outline_color {}; /* outline color RGBA */
    glm::vec4 params {};        /* .x = atlas_index, .y = outline_width, .z = softness, .w = boldness */
    glm::vec4 shadow_params {}; /* .xy = shadow_offset, .z = shadow_softness, .w = unused */
    glm::vec4 shadow_color {};  /* shadow color RGBA */
    glm::vec4 glow_params {};   /* .x = glow_width, .yzw = unused */
    glm::vec4 glow_color {};    /* glow color RGBA */
};

class UiPipeline {
   private:
    /* Image Rendering */
    Buffer images_buffer {};
    Buffer image_vertex_buffer {};
    Sampler image_sampler {};
    uint32_t image_count = 0;

    /* Text Rendering */
    Buffer glyphs_buffer {};
    Buffer glyph_vertex_buffer {};
    Sampler text_sampler {};
    uint32_t glyph_count = 0;

   public:
    void init(GPUAdapter& gpu);
    void enqueue(RenderGraph& render_graph, RenderView& render_view);

    void enqueue_images(RenderGraph& render_graph, RenderView& render_view);
    void enqueue_text(RenderGraph& render_graph, RenderView& render_view);

    void deinit(GPUAdapter& gpu);

    UiPipeline() = default;
    ~UiPipeline() = default;

    /* Cannot be copied */
    UiPipeline(const UiPipeline&) = delete;
    UiPipeline& operator=(const UiPipeline&) = delete;

    bool render_ui_pipeline = true;
    bool render_text = true;
};

}  // namespace tmt