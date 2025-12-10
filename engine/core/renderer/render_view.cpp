#include "render_view.hpp"

#include <graphite/imgui.hh>
#include <graphite/vram_bank.hh>
#include <graphite/gpu_adapter.hh>
#include <graphite/render_graph.hh>

#include "core/logger.hpp"

namespace tmt {

void RenderView::init() {
}

void RenderView::update() {

}

void RenderView::uninit() {

}

BindHandle RenderView::get_render_image() const { 
#ifdef THERMITE_EDITOR
    return viewport_image;
#else
    return render_target;
#endif  // THERMITE_EDITOR
}

void RenderView::set_viewport_size(uint32_t width, uint32_t height) {
    if (width != gpu_view.resolution.x || height != gpu_view.resolution.y) {
        gpu_view.resolution = {width, height};

        if (const Result r = gpu.get_vram_bank().resize_texture(viewport_texture, {width, height, 0}); r.is_err()) {
            Log::error(Log::Scope::RENDERER, "failed to resize the viewport texture.\nreason: {}", r.unwrap_err().c_str());
        } else {
            Log::info(Log::Scope::RENDERER, "viewport texture has been resized.");
        }

        imgui->remove_image(viewport_image);
        imgui_viewport = imgui->add_image(viewport_image);
    }
}

}  // namespace tmt
