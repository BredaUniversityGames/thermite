#include "console.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/input/input.hpp"
#include "extern/imgui-console/include/csys/arguments.h"
using namespace tmt;
void Console::register_commands() {
    register_command(
        "create", "Create entity: create <name>",
        [](const std::string& name) {
            Entity entity = engine.ecs.create_entity(name);
            Log::info("Created '{}' [{}]", name, static_cast<uint32_t>(entity));
        },
        csys::Arg<csys::String>("name")
    );

    register_command(
        "destroy", "Destroy entity: destroy <name>",
        [](const std::string& name) {
            auto& registry = engine.ecs.get_registry();
            for (auto entity : registry.view<Name>()) {
                if (registry.get<Name>(entity).name == name) {
                    engine.ecs.destroy_entity(entity);
                    Log::info("Destroyed '{}'", name);
                    return;
                }
            }
            Log::error("Entity '{}' not found", name);
        },
        csys::Arg<csys::String>("name")
    );

    register_command("list", "List all entities", []() {
        auto& registry = engine.ecs.get_registry();
        for (auto entity : registry.view<Name>()) {
            Log::info("[{}] {}", static_cast<uint32_t>(entity), registry.get<Name>(entity).name);
        }
    });

    register_command(
        "warp_mouse", "Moves the cursor instantly to the given position relative to the window",
        [](float x, float y) {
            engine.input.warp_mouse({x, y});
            Log::info("Mouse position moved to ({}, {})", x, y);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y")
    );
    // Drawing state
    register_command(
        "draw_color", "Sets the draw color: draw_color <r> <g> <b>", [](float r, float g, float b) { engine.polyline.use_color(r, g, b); }, csys::Arg<float>("r"), csys::Arg<float>("g"),
        csys::Arg<float>("b")
    );

    register_command("draw_width", "Sets the line width: draw_width [width=1]", [](float width) { engine.polyline.use_line_width(width); }, csys::Arg<float>("width", 1.0f));

    // Shape commands
    register_command(
        "draw_line", "Draws a line between two points: draw_line <x1> <y1> <z1> <x2> <y2> <z2> [time=5]",
        [](float x1, float y1, float z1, float x2, float y2, float z2, float time) { engine.polyline.draw_line({x1, y1, z1}, {x2, y2, z2}, time); }, csys::Arg<float>("x1"),
        csys::Arg<float>("y1"), csys::Arg<float>("z1"), csys::Arg<float>("x2"), csys::Arg<float>("y2"), csys::Arg<float>("z2"), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_circle", "Draws a circle at position: draw_circle <x> <y> <z> <radius> [segments=32] [time=5]",
        [](float x, float y, float z, float radius, int segments, float time) { engine.polyline.draw_circle({x, y, z}, radius, segments, time); }, csys::Arg<float>("x"), csys::Arg<float>("y"),
        csys::Arg<float>("z"), csys::Arg<float>("radius"), csys::Arg<int>("segments", 32), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_sphere", "Draws a wireframe sphere at position: draw_sphere <x> <y> <z> <radius> [segments=16] [time=5]",
        [](float x, float y, float z, float radius, int segments, float time) { engine.polyline.draw_sphere({x, y, z}, radius, segments, time); }, csys::Arg<float>("x"), csys::Arg<float>("y"),
        csys::Arg<float>("z"), csys::Arg<float>("radius"), csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_aabb", "Draws an AABB: draw_aabb <minX> <minY> <minZ> <maxX> <maxY> <maxZ> [time=5]",
        [](float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float time) { engine.polyline.draw_aabb({minX, minY, minZ}, {maxX, maxY, maxZ}, time); },
        csys::Arg<float>("minX"), csys::Arg<float>("minY"), csys::Arg<float>("minZ"), csys::Arg<float>("maxX"), csys::Arg<float>("maxY"), csys::Arg<float>("maxZ"),
        csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_arrow", "Draws an arrow from position: draw_arrow <x> <y> <z> <dirX> <dirY> <dirZ> [length=1] [time=5]",
        [](float x, float y, float z, float dirX, float dirY, float dirZ, float length, float time) {
            engine.polyline.draw_arrow({x, y, z}, glm::normalize(glm::vec3 {dirX, dirY, dirZ}), length, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("length", 1.0f),
        csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_cone", "Draws a wireframe cone: draw_cone <x> <y> <z> <dirX> <dirY> <dirZ> [angle=45] [length=1] [segments=16] [time=5]",
        [](float x, float y, float z, float dirX, float dirY, float dirZ, float angle, float length, int segments, float time) {
            engine.polyline.draw_cone({x, y, z}, glm::normalize(glm::vec3 {dirX, dirY, dirZ}), angle, length, segments, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("angle", 45.0f),
        csys::Arg<float>("length", 1.0f), csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_tube", "Draws a wireframe tube between two points: draw_tube <x1> <y1> <z1> <x2> <y2> <z2> [radius=0.1] [segments=16] [time=5]",
        [](float x1, float y1, float z1, float x2, float y2, float z2, float radius, int segments, float time) {
            engine.polyline.draw_tube({x1, y1, z1}, {x2, y2, z2}, radius, segments, time);
        },
        csys::Arg<float>("x1"), csys::Arg<float>("y1"), csys::Arg<float>("z1"), csys::Arg<float>("x2"), csys::Arg<float>("y2"), csys::Arg<float>("z2"), csys::Arg<float>("radius", 0.1f),
        csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_obb", "Draws an OBB: draw_obb <x> <y> <z> <hx> <hy> <hz> [axisX=0] [axisY=1] [axisZ=0] [angle=0] [time=5]",
        [](float x, float y, float z, float hx, float hy, float hz, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 {axisX, axisY, axisZ});
            engine.polyline.draw_obb({x, y, z}, {hx, hy, hz}, glm::angleAxis(angle, axis), time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("hx"), csys::Arg<float>("hy"), csys::Arg<float>("hz"), csys::Arg<float>("axisX", 0.0f),
        csys::Arg<float>("axisY", 1.0f), csys::Arg<float>("axisZ", 0.0f), csys::Arg<float>("angle", 0.0f), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_bone", "Draws a bone: draw_bone <x> <y> <z> [length=1] [axisX=0] [axisY=1] [axisZ=0] [angle=0] [time=5]",
        [](float x, float y, float z, float length, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 {axisX, axisY, axisZ});
            engine.polyline.draw_bone({x, y, z}, glm::angleAxis(angle, axis), length, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("length", 1.0f), csys::Arg<float>("axisX", 0.0f), csys::Arg<float>("axisY", 1.0f),
        csys::Arg<float>("axisZ", 0.0f), csys::Arg<float>("angle", 0.0f), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_text", "Draws billboard text: draw_text <x> <y> <z> <text> [size=1] [time=5]",
        [](float x, float y, float z, const std::string& text, float size, float time) { engine.polyline.draw_text({x, y, z}, text, size, time); }, csys::Arg<float>("x"),
        csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<csys::String>("text"), csys::Arg<float>("size", 1.0f), csys::Arg<float>("time", 5.0f)
    );
}
