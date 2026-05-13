#include "screenshot.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/pipelines/post_process_pipeline.hpp"

namespace tmt {

void ScreenshotWindow::on_inspect() {
    //ImGui::DragFloat("Hue", &engine.renderer.post_process_pipeline.hsl_packed.x, 0.01f, -180.0f, 180.0f);
    //ImGui::SliderFloat("Saturation", &engine.renderer.post_process_pipeline.hsl_packed.y, -100.0f, 100.0f);
    //ImGui::SliderFloat("Lightness", &engine.renderer.post_process_pipeline.hsl_packed.z, -100.0f, 100.0f);
    
    if (ImGui::Button("Take Screenshot")) {
        engine.renderer.take_screenshot = true;
    }
}

}  // namespace tmt
