#include "lights.hpp"

#include "editor/editor.hpp"
#include "editor/shared/colors.hpp"
#include "editor/windows/hierarchy.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/light.hpp"

namespace tmt {

/* Size of the reticle used for overlays. (diamond shape in the center) */
constexpr float OVERLAY_RETICLE_SIZE = 0.01f;

/* Draw sphere light overlay. */
inline void sphere_light_gizmo(Transform& transform, SphereLight light, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;

    /* Draw the radii */
    engine.polyline.use_color(color);
    engine.polyline.draw_circle(pos, light.source_radius, 32u);      /* Source radius */
    engine.polyline.draw_circle(pos, light.attenuation_radius, 32u); /* Attenuation radius */

    /* Draw vertical line */
    engine.polyline.use_color(color, 0.5f);
    engine.polyline.draw_line(pos, pos * glm::vec3(1.0f, 0.0f, 1.0f));
}

constexpr float SPOTLIGHT_CONE_SIZE = 10.0f;

/* Draw spot light overlay. */
inline void spot_light_gizmo(Transform& transform, const SpotLight& light, bool selected) {
    /* Get the position of the light source */
    const glm::vec3 pos = transform.get_world_position();
    const glm::vec3 dir = transform.get_forward();
    const glm::vec3 color = selected ? colors::SELECTED : colors::DARK;

    /* Draw the source radius */
    engine.polyline.use_color(color);
    engine.polyline.draw_circle(pos, light.source_radius, 32u); /* Source radius */

    /* Draw the attenuation distance using a line */
    engine.polyline.use_color(color, 0.5f);
    engine.polyline.draw_line(pos, pos + dir * light.attenuation_distance); /* Attenuation distance */

    /* Find the cone radius and length based on the cone size and angle */
    const float safe_angle = glm::tan(glm::min(light.beam_angle * 0.5f, glm::pi<float>() * 0.5f - 0.001f));
    const float cone_radius = SPOTLIGHT_CONE_SIZE * glm::sin(light.beam_angle * 0.5f);
    const float cone_length = glm::max(0.01f, glm::sqrt(SPOTLIGHT_CONE_SIZE * SPOTLIGHT_CONE_SIZE - cone_radius * cone_radius));

    /* Draw the cone of the spot light */
    engine.polyline.use_color(color);
    engine.polyline.draw_cone(pos, dir, light.beam_angle * 0.5f, cone_length, 32u);

    /* Draw the inner circle of the cone, to show it's blend radius */
    const float cone_inner_radius = cone_length * safe_angle * (1.0f - light.spot_blend);
    const glm::vec3 cone_end = pos + dir * cone_length;
    engine.polyline.draw_world_circle(cone_end, dir, cone_inner_radius, 32u);
}

/* Draw the overlay for light components. */
void draw_lights_overlay() {
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
        engine.polyline.draw_circle(light_pos, distance * OVERLAY_RETICLE_SIZE, 4u);

        switch (light.type) {
            /* Sphere light gizmo */
            case LightType::SPHERE_LIGHT:
                sphere_light_gizmo(transform, std::get<SphereLight>(light.light), selected);
                break;
            /* Sphere light gizmo */
            case LightType::SPOT_LIGHT:
                spot_light_gizmo(transform, std::get<SpotLight>(light.light), selected);
                break;
            default:
                break;
        }
    }
}

}  // namespace tmt
