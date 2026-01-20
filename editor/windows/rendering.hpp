#pragma once

#include "engine/tools/serializer.hpp"
#include "editor/core/window.hpp"

namespace tmt {

// struct RenderingSettings {
    
// };

class Rendering : public IWindow {
    /* Render all editor gizmos. */
    void render_gizmos();

   public:
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
    void display() override;
    constexpr std::string get_title() const override { return ICON_MS_MOVIE " Rendering"; }
};

}  // namespace tmt

// JSON_REFLECT(tmt::RenderingSettings);
