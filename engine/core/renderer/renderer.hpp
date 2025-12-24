#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <graphite/imgui.hh>

#include "render_view.hpp"
#include "scene_view.hpp"

class GPUAdapter;
class RenderGraph;
class VRAMBank;

namespace tmt {

/* Renderer display mode. */
enum class DisplayMode : uint32_t {
    Default = 0u,
    Visibility, /* Visualize visibility buffer data. */
    Depth, /* Visualize geometric depth data. (0..100) */
    Normals, /* Visualize geometric normal data. */
    Albedo, /* Visualize material albedo. */
};
static const DisplayMode DISPLAY_MODES[] { DisplayMode::Default, DisplayMode::Visibility, DisplayMode::Depth, DisplayMode::Normals, DisplayMode::Albedo };
static const char* DISPLAY_MODE_NAMES[] { "Default", "Visibility", "Depth", "Normals", "Albedo" };

class Renderer {
    GPUAdapter& gpu;
    RenderGraph& render_graph;

    /* Pipelines */
    class DebugPipeline& debug_pipeline;
    class GeometryPipeline& geometry_pipeline;

    /* Debug camera */
    Transform debug_transform;
    Camera debug_camera;

   public:
    /* Rendering views */
    RenderView render_view {};
    SceneView scene_view {};

#ifdef THERMITE_EDITOR
    ImGUI* imgui = nullptr;
#endif
    /* Display mode for debugging */
    DisplayMode display_mode = DisplayMode::Default;

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void update();
    void end();

    void draw_line(const glm::vec3 start, const glm::vec3 end, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const float time = 0.0f);

    /* Draw a circle in 3D space. Axis_a and axis_b define the plane that the circle lays in */
    void draw_circle(
        const glm::vec3 center, const float radius, const glm::vec3 axis_a, const glm::vec3 axis_b, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, int segments = 16, const float time = 0.0f
    );

    /* Needs at least 4 segments and 2 rings */
    void draw_sphere(const glm::vec3 center, const float radius, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, int rings = 4, int segments = 16, const float time = 0.0f);

    /* head_angle is in degrees */
    void draw_arrow(
        const glm::vec3 start, glm::vec3 dir, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, float length = 1.0f, float head_length = 0.1f, float head_angle = 10.0f, const float time = 0.0f
    );

    void draw_cross(const glm::vec3 center, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const float size = 0.5f, const glm::quat rot = glm::quat(), const float time = 0.0f);

    /* If rot is not set, we draw an aabb */
    void draw_obb(
        const glm::vec3 center, const float width, const float height, const float depth, const glm::vec3 color = {1.0f, 0.0f, 0.0f}, const glm::quat rot = glm::quat(), const float time = 0.0f
    );

    /* Get a reference to the VRAM bank, for creating and destroying graphics resources. */
    VRAMBank& vram_bank();

    Transform& get_debug_transform() { return debug_transform; }
    Camera& get_debug_camera() { return debug_camera; }

#ifdef THERMITE_EDITOR
    void set_imgui(ImGUI* imgui);
#endif
};

}  // namespace tmt
