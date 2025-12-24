#include "render_view.hpp"

#include <graphite/imgui.hh>
#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>

#include "engine.hpp"
#include "renderer.hpp"

#include "core/logger.hpp"
#include "core/window.hpp"
#include "core/ecs.hpp"

#include "core/components/camera.hpp"
#include "core/components/transform.hpp"

namespace tmt {

void RenderView::init() {
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Initialize the Render Target */
    const TargetDesc target {engine.window.get_window_handle()};
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize render target.\nreason: {}", r.unwrap_err());
        return;
    } else {
        render_target = r.unwrap();
    }

    /* Viewport Texture */
    if (const Result r = bank.create_texture(
            TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::Storage, TextureFormat::RGBA8Unorm, {(uint32_t)engine.window.width, (uint32_t)engine.window.height, 0}
        );
        r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize attachment texture.\nreason: {}", r.unwrap_err());
        return;
    } else
        viewport.texture = r.unwrap();
    /* Viewport Image */
    if (const Result r = bank.create_image(viewport.texture); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to initialize attachment image.\nreason: {}", r.unwrap_err());
        return;
    } else
        viewport.image = r.unwrap();

    /* Create the active render view buffer */
    if (const Result r = bank.create_buffer(BufferUsage::Constant | BufferUsage::TransferDst, sizeof(RenderView)); r.is_err()) {
        Log::error(Log::Scope::RENDERER, "failed to create render view buffer.\nreason: {}", r.unwrap_err().c_str());
        return;
    } else {
        render_view_buffer = r.unwrap();
    }

    /* Create the visibility buffer */
    const Size3D view_size { gpu_view.resolution.x, gpu_view.resolution.y };
    vbuffer.texture = bank.create_texture(TextureUsage::Storage, TextureFormat::RG32Uint, view_size).expect("failed to create vbuffer texture.");
    vbuffer.image = bank.create_image(vbuffer.texture).expect("failed to create vbuffer image.");

    /* Create the illuminance buffer */
    ibuffer.texture = bank.create_texture(TextureUsage::Storage, TextureFormat::RG11B10Ufloat, view_size).expect("failed to create ibuffer texture.");
    ibuffer.image = bank.create_image(vbuffer.texture).expect("failed to create ibuffer image.");
}

void RenderView::update() {
#ifndef THERMITE_EDITOR
    gpu_view.resolution = glm::uvec2(engine.window.width, engine.window.height);
#endif  // !THERMITE_EDITOR

    /* Resize the render target if the window was resized */
    if (engine.window.resized) {
        VRAMBank& bank = engine.renderer.vram_bank();

        /* Resize the render target */
        if (const Result r = bank.resize_render_target(render_target, gpu_view.resolution.x, gpu_view.resolution.y); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to resize the swapchain.\nreason: {}", r.unwrap_err().c_str());
        }
        engine.window.resized = false;
    }
}

void RenderView::update_gpu_view(RenderGraph& render_graph, const Camera& camera, const Transform& transform) {
    const float aspect_ratio = (float)gpu_view.resolution.x / (float)gpu_view.resolution.y;

    /* Iterate over all cameras to find an active one to use as render view */
    glm::mat4 p = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);
    const glm::mat4 v = glm::inverse(transform.get_world_matrix());
    p[1][1] *= -1.0f;
    gpu_view.world_to_clip = p * v;
    gpu_view.clip_to_world = glm::inverse(gpu_view.world_to_clip);
    gpu_view.origin = glm::vec4(transform.get_world_position(), 0.0f);

    /* Upload the active render view */
    render_graph.upload_buffer(render_view_buffer, &gpu_view, 0u, sizeof(GpuView));
}

void RenderView::deinit() {
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Destroy screen buffers */
    bank.destroy(vbuffer.image);
    bank.destroy(vbuffer.texture);
    bank.destroy(ibuffer.image);
    bank.destroy(ibuffer.texture);
    bank.destroy(viewport.texture);
    bank.destroy(viewport.image);

    bank.destroy(render_view_buffer);
    bank.destroy(render_target);
}

BindHandle RenderView::get_render_image() const {
#ifdef THERMITE_EDITOR
    return viewport.image;
#else
    return render_target;
#endif  // THERMITE_EDITOR
}

void RenderView::set_viewport_size(uint32_t width, uint32_t height) {
    if (width != gpu_view.resolution.x || height != gpu_view.resolution.y) {
        height = height <= 0 ? 1 : height;
        gpu_view.resolution = {width, height};
        resize_textures();

#ifdef THERMITE_EDITOR
        engine.renderer.imgui->remove_image(viewport.image);
        imgui_viewport = engine.renderer.imgui->add_image(viewport.image);
#endif
    }
}

void RenderView::resize_textures() {
    const Size3D view_size { gpu_view.resolution.x, gpu_view.resolution.y };
    VRAMBank& bank = engine.renderer.vram_bank();

    /* Resize the screen buffers */
    bank.resize_texture(viewport.texture, view_size).expect("failed to resize viewport texture.");
    bank.resize_texture(vbuffer.texture, view_size).expect("failed to resize vbuffer texture.");
    bank.resize_texture(ibuffer.texture, view_size).expect("failed to resize ibuffer texture.");
}

}  // namespace tmt
