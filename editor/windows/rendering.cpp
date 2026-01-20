#include "rendering.hpp"

#include "editor/editor.hpp"
#include "editor/shared/colors.hpp"
#include "editor/windows/hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/render_view.hpp"

#include "engine/core/components/light.hpp"

namespace tmt {

void Rendering::on_editor_start() {}

void Rendering::on_editor_update(const tmt::FrameData&) {
    render_gizmos();
}

void Rendering::on_editor_end() {}

void Rendering::display() {}

/* Size of the reticle used for gizmos. (diamond shape in the center) */
constexpr float GIZMO_RETICLE_SIZE = 0.01f;

/* Draw sphere light gizmo. */
inline void sphere_light_gizmo(Transform& transform, SphereLight light) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();

    /* Draw the radii */
    engine.polyline.use_color(colors::DARK);
    engine.polyline.draw_circle(pos, light.source_radius, 32u); /* Source radius */
    engine.polyline.draw_circle(pos, light.attenuation_radius, 32u); /* Attenuation radius */

    /* Draw vertical line */
    engine.polyline.use_color(colors::DARK, 0.5f);
    engine.polyline.draw_line(pos, pos * glm::vec3(1.0f, 0.0f, 1.0f));
}

/* Draw spot light gizmo. */
inline void spot_light_gizmo(Transform& transform, const SpotLight& light) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 dir = transform.get_forward();

    /* Draw the source radius */
    engine.polyline.use_color(colors::DARK);
    engine.polyline.draw_circle(pos, light.source_radius, 32u); /* Source radius */

    /* Draw the attenuation distance using a line */
    engine.polyline.use_color(colors::DARK, 0.5f);
    engine.polyline.draw_line(pos, pos + dir * light.attenuation_distance); /* Attenuation distance */

    /* Draw the end circle of the cone */
    const float cone_length = (3.2f - light.beam_angle) * 5.0f;
    const glm::vec3 cone_end = pos + dir * cone_length;
    const float cone_end_radius = cone_length * glm::tan(light.beam_angle * 0.5f);
    // engine.polyline.draw_world_circle(cone_end, dir, cone_end_radius, 32u);
    engine.polyline.draw_cone(pos, dir, light.beam_angle, cone_length, 32u);
    
    /* Draw the inner circle of the cone, to show it's blend radius */
    const float cone_inner_radius = cone_end_radius * (1.0f - light.spot_blend);
    engine.polyline.draw_world_circle(cone_end, dir, cone_inner_radius, 32u);
}

/* Draw gizmos for light components. */
inline void light_gizmos() {
    /* Capture all lights in the scene */
    const entt::basic_group group = engine.ecs.get_registry().group<const Light>(entt::get<Transform>);

    /* Get the camera position is world-space */
    const glm::vec3 camera_pos = engine.renderer.render_view.gpu_view.origin;

    /* 1.5px line width */
    engine.polyline.use_line_width(1.5f);
    
    /* Iterate over all lights */
    for (auto&& [entity, light, transform] : group.each()) {
        /* Get the position of the light source */
        const glm::vec3 light_pos = transform.get_world_position();
        /* Check if this sphere light is selected */
        const bool selected = editor.windows.get<Hierarchy>().is_entity_selected(entity);

        /* Draw the reticle */
        engine.polyline.use_color(selected ? colors::SELECTED : colors::DARK);
        const float distance = glm::distance(camera_pos, light_pos);
        engine.polyline.draw_circle(light_pos, distance * GIZMO_RETICLE_SIZE, 4u);

        switch (light.type) {
            /* Sphere light gizmo */
            case LightType::SPHERE_LIGHT:
                sphere_light_gizmo(transform, std::get<SphereLight>(light.light));
                break;
            /* Sphere light gizmo */
            case LightType::SPOT_LIGHT:
                spot_light_gizmo(transform, std::get<SpotLight>(light.light));
                break;
            default: break;
        }
    }
}

void Rendering::render_gizmos() {
    light_gizmos();
}

}  // namespace tmt
