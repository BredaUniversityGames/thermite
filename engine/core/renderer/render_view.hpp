#pragma once

#include <glm/glm.hpp>

#include <graphite/resources/handle.hh>

#include "engine/core/entity.hpp"

#include "engine/core/components/camera.hpp"
#include "engine/core/components/transform.hpp"

class RenderGraph;

namespace tmt {

/* Represents a renderable view in the scene. (used on the GPU) */
struct GpuView {
    /* World-space to clip-space transformation matrix. */
    glm::mat4 world_to_clip {};
    /* Clip-space to world-space transformation matrix. */
    glm::mat4 clip_to_world {};
    /* Origin of the view in world-space. */
    glm::vec4 origin {};
    /* Resolution of the view in pixels. */
    glm::uvec2 resolution {1, 1};
    /* Index of the current frame. */
    glm::uint frame_index = 0u;
};

/* Screen buffer resource. */
struct ScreenBuffer {
    Texture texture {};
    Image image {};
};

struct RenderView {
    RenderView() = default;
    ~RenderView() = default;

    void init();
    void update();
    void update_gpu_view(RenderGraph& render_graph, const Camera& camera, const Transform& transform);
    void deinit();

    /* If we are running the editor, we render to the viewport_image, otherwise we render directly to the render_target */
    BindHandle get_render_image() const;

    void set_viewport_size(uint32_t width, uint32_t height);

    /* Screen buffers */
    ScreenBuffer vbuffer {}; /* Visibility buffer (WxH, 6->8 bytes) */
    ScreenBuffer ibuffer {}; /* Illuminance buffer (WxH, 4 bytes) */

    /* Cache buffers */
    Buffer cache {}; /* Illuminance cache buffer */
    Buffer cache_info {}; /* Illuminance cache info buffer */
    Buffer macrofacets {}; /* Macro-facets buffer */

    /* Renderer output */
    ScreenBuffer viewport {};
    u64 imgui_viewport {};

    /* Render view constant buffer */
    Buffer render_view_buffer {};
    GpuView gpu_view {};
    
    /* (Final) Render target */
    RenderTarget render_target {};
    glm::uint frame_counter = 0u;

   private:
    void resize_textures();
};

}  // namespace tmt
