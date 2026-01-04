#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>

#include <graphite/resources/handle.hh>

#include "engine/core/renderer/render_view.hpp"
#include "engine/events/engine.hpp"

class GPUAdapter;
class RenderGraph;

namespace tmt {

/* All data that makes up a polyline segment. */
struct PolylineSegment {
    glm::vec3 point_a {0.0f, 0.0f, 0.0f};
    glm::vec3 point_b {0.0f, 0.0f, 0.0f};
    glm::vec4 color {1.0f, 0.0f, 0.0f, 1.0f};
    float line_width = 0.1f;
    PolylineSegment() = default;
    PolylineSegment(glm::vec3 point_a, glm::vec3 point_b, glm::vec4 color, float line_width) : point_a(point_a), point_b(point_b), color(color), line_width(line_width) {};
};

/* Polyline segment with a timer attached. */
struct TimedPolylineSegment {
    PolylineSegment line {};
    float timer = 0.0f; /* Seconds */
    TimedPolylineSegment() = default;
    TimedPolylineSegment(glm::vec3 point_a, glm::vec3 point_b, glm::vec4 color, float line_width, float time) : line(point_a, point_b, color, line_width), timer(time) {};
};

/* Maximum number of polylines that can be drawn per frame. */
constexpr uint32_t MAX_POLYLINES = 1u << 14;

/**
 * Pipeline for drawing polygonal lines, used for debugging and editor visualizations.
 */
class PolylinePipeline : public OnEngineUpdate {
   private:
    /* GPU line segments buffer */
    Buffer line_buffer {};

    /* CPU buffers for timed and non-timed line segments */
    std::vector<TimedPolylineSegment> timed_lines {};
    std::vector<PolylineSegment> immediate_lines {};

    /* Number of active line segments */
    uint32_t line_segment_count = 0u;

    /* Polyline material state */
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float line_width = 0.1f;

    void init(GPUAdapter& gpu);
    void on_engine_update(const FrameData& time) override;
    void enqueue(RenderGraph& render_graph, RenderView render_view);
    void deinit(GPUAdapter& gpu);

   public:
    PolylinePipeline() = default;
    ~PolylinePipeline() = default;

    /* Cannot be copied */
    PolylinePipeline(const PolylinePipeline&) = delete;
    PolylinePipeline& operator=(const PolylinePipeline&) = delete;

    /* Use the given color when drawing subsequent lines. */
    inline void use_color(glm::vec4 new_color) { color = new_color; };
    inline void use_color(float r, float g, float b, float a = 1.0f) { color = glm::vec4(r, g, b, a); };
    /* Use the given line width when drawing subsequent lines. */
    inline void use_line_width(float new_width, bool screen_space = false) { line_width = screen_space ? -abs(new_width) : abs(new_width); };

    /**
     * @brief Draw a polyline.
     * @param a The starting point of the polyline in world-space.
     * @param b The end point of the polyline in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_line(glm::vec3 a, glm::vec3 b, float time = 0.0f);

    /**
     * @brief Draw a polyline circle which always faces the camera.
     * @param origin The center point of the circle in world-space.
     * @param radius The radius of the circle in world-space.
     * @param segments The number of line segments which make up the circle.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_circle(glm::vec3 origin, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline sphere using 3 orthogonal circles.
     * @param origin The center point of the sphere in world-space.
     * @param radius The radius of the sphere in world-space.
     * @param segments The number of line segments which make up each of the orthogonal circles.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_sphere(glm::vec3 origin, float radius, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline arrow which always faces the camera.
     * @param origin The origin point of the arrow in world-space.
     * @param dir The direction of the arrow in world-space.
     * @param length The length of the arrow in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_arrow(glm::vec3 origin, glm::vec3 dir, float length, float time = 0.0f);

    /**
     * @brief Draw a polyline axis aligned bounding box.
     * @param min The minimum point of the AABB in world-space.
     * @param max The maximum point of the AABB in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_aabb(glm::vec3 min, glm::vec3 max, float time = 0.0f);

    /**
     * @brief Draw a polyline oriented bounding box.
     * @param origin The center point of the OBB in world-space.
     * @param half_extent Half of the extent of the OBB in local-space using world-space units.
     * @param rot The rotation of the OBB in world-space.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_obb(glm::vec3 origin, glm::vec3 half_extent, glm::quat rot, float time = 0.0f);

    /**
     * @brief Draw a polyline cone with side lines that always face the camera.
     * @param origin The origin point of the cone in world-space.
     * @param dir The direction of the cone in world-space.
     * @param angle The half angle of the cone in radians.
     * @param length The length of the cone in world-space.
     * @param segments The number of line segments which make up the circle at the end of the cone.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_cone(glm::vec3 origin, glm::vec3 dir, float angle, float length, uint32_t segments = 32u, float time = 0.0f);

    /**
     * @brief Draw a polyline tube with side lines that always face the camera.
     * @param a The starting point of the tube in world-space.
     * @param b The end point of the tube in world-space.
     * @param radius The radius of the tube in world-space.
     * @param segments The number of line segments which make up each of the end circles.
     * @param time Optional timer for how long the polyline should stay alive. *(default is 1 frame)*
     */
    void draw_tube(glm::vec3 a, glm::vec3 b, float radius, uint32_t segments = 32u, float time = 0.0f);

    friend class Renderer;
};

}  // namespace tmt
