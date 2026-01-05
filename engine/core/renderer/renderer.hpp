#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>

#include "render_view.hpp"
#include "scene_view.hpp"

class GPUAdapter;
class RenderGraph;
class VRAMBank;
class ImGUI;

namespace tmt {

/* Renderer display mode. */
enum class DisplayMode : uint32_t {
    Default = 0u,
    Steps, /* Visualize visibility pass step count. (0..128) */
    Visibility, /* Visualize visibility buffer data. */
    Depth, /* Visualize geometric depth data. (0..100) */
    Normals, /* Visualize geometric normal data. */
    Albedo, /* Visualize material albedo. */
};
static const DisplayMode DISPLAY_MODES[] { DisplayMode::Default, DisplayMode::Steps, DisplayMode::Visibility, DisplayMode::Depth, DisplayMode::Normals, DisplayMode::Albedo };
static const char* DISPLAY_MODE_NAMES[] { "Default", "Steps (0..128)", "Visibility", "Depth (0..100)", "Normals", "Albedo" };

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    /* Debug camera */
    Transform debug_transform;
    Camera debug_camera;

   public:
    /* Rendering views */
    RenderView render_view {};
    SceneView scene_view {};

    /* Display mode for debugging */
    DisplayMode display_mode = DisplayMode::Default;

    /* Pipelines */
    class GeometryPipeline& geometry_pipeline;
    class DiPipeline& di_pipeline;
    class PolylinePipeline& polyline_pipeline;

#ifdef THERMITE_EDITOR
    ImGUI* imgui = nullptr;
#endif

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    VRAMBank& vram_bank();

    Transform& get_debug_transform() { return debug_transform; }
    Camera& get_debug_camera() { return debug_camera; }

#ifdef THERMITE_EDITOR
    void set_imgui(ImGUI* imgui);
#endif
};

}  // namespace tmt
