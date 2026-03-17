#include "rendering.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/render_view.hpp"

#include "editor/overlays/lights.hpp"

namespace tmt {

void Rendering::on_draw_lines() const {
    draw_lights_overlay();
}

}  // namespace tmt
