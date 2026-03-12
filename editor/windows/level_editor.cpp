#include "level_editor.hpp"

#include "editor/editor.hpp"
#include "editor/windows/viewport.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/engine.hpp"
#include "imgui/imgui/imgui.h"
#include "engine/core/polyline.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

void tmt::LevelEditor::display() {
    ImGui::Checkbox("Enable level editing", &enable_editing);
    ImGui::Checkbox("Mirror X", &mirror_x);
    ImGui::InputFloat("Cell size", &cell_size);

    static glm::vec4 cell_color(1.f);
    ImGui::ColorPicker4("Cell color", &cell_color[0]);

    auto& viewport = editor.windows[Editor::Mode::SCENE].get<Viewport>();
    ImDrawList* draw_list = viewport.get_drawlist();

    if (enable_editing) {
        for (auto [coord, cell] : cells) {
            draw_cell(draw_list, coord_to_world(coord), cell);
        }

        const float max_alpha = 0.2f;
        const float diff_amplitude = 0.15f;
        float alpha = max_alpha - diff_amplitude * (glm::sin(highlight_timer * 2.f) + 1.f) * 0.5f;

        if(viewport.get_is_hovered())
        {
            for (const auto& coord : placing_coords) {
                draw_cell(draw_list, coord_to_world(coord), Cell { .fill_color = glm::vec4(1.f, 1.f, 1.f, alpha), .edge_color = glm::vec4(1.f) });
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                for (const auto& coord : placing_coords) {
                    cells.try_emplace(coord, Cell { .fill_color = cell_color - glm::vec4(0.1f), .edge_color = cell_color });
                }
            }
        }
    }
}

void tmt::LevelEditor::on_editor_start() {};

void tmt::LevelEditor::on_editor_update(const tmt::FrameData& time) {
    if (enable_editing) {
        auto& viewport = editor.windows[Editor::Mode::SCENE].get<Viewport>();
        auto& camera = engine.renderer.get_debug_camera();
        auto& camera_transform = engine.renderer.get_debug_transform();

        auto view = glm::inverse(camera_transform.get_world_matrix());
        float aspect = viewport.get_width() / viewport.get_height();
        auto project = glm::perspective(glm::radians(camera.fov), aspect, 0.01f, 1000.f);
        auto mouse_pos = viewport.get_mouse_pos();

        viewport_context.height = viewport.get_height();
        viewport_context.width = viewport.get_width();
        viewport_context.p = viewport.get_window_pos();
        viewport_context.view_project = project * view;

        glm::vec4 viewport4 { 0.f, 0.f, viewport.get_width(), viewport.get_height() };

        glm::vec3 near = { mouse_pos.x, viewport_context.height - mouse_pos.y, 0.0f };
        glm::vec3 far { mouse_pos.x, viewport_context.height - mouse_pos.y, 1.f };

        glm::vec3 ray_origin = glm::unProject(near, view, project, viewport4);
        glm::vec3 ray_far = glm::unProject(far, view, project, viewport4);
        glm::vec3 ray_dir = glm::normalize(ray_far - ray_origin);

        float t = -(ray_origin.y / ray_dir.y);

        placing_coords.clear();
        if (t > 0.f) {
            // possible xz plane intersection
            place_xz_pos = ray_origin + ray_dir * t;
            placing_coords.push_back(world_to_coord(place_xz_pos));

            if (mirror_x) {
                glm::vec3 place_xz_mirror = { -place_xz_pos.x, 0.f, place_xz_pos.z };
                placing_coords.push_back(world_to_coord(place_xz_mirror));
            }
        }

        highlight_timer += time.delta_time;
    }
}

void LevelEditor::draw_cell(ImDrawList* draw_list, glm::vec3 pos, const Cell& cell) {
    float extent = cell_size * 0.5f;
    glm::vec3 local_vertices[8] = { glm::vec3(-extent, -extent, -extent), glm::vec3(extent, -extent, -extent), glm::vec3(extent, extent, -extent), glm::vec3(-extent, extent, -extent),
                                    glm::vec3(-extent, -extent, extent),  glm::vec3(extent, -extent, extent),  glm::vec3(extent, extent, extent),  glm::vec3(-extent, extent, extent) };

    ImVec2 screen_vertices[8];
    for (int i = 0; i < 8; i++) {
        glm::vec4 p = viewport_context.view_project * glm::vec4(local_vertices[i] + pos, 1.f);

        if (p.w < 0.f) {
            return;
        }

        p *= 0.5f / p.w;
        p += glm::vec4(0.5f, 0.5f, 0.f, 0.f);
        p.y = 1.f - p.y;
        p.x *= viewport_context.width;
        p.y *= viewport_context.height;
        p.x += viewport_context.p.x;
        p.y += viewport_context.p.y;

        screen_vertices[i] = { p.x, p.y };
    }

    // 6 faces of the cube
    static const int faces[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 }, { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };

    ImU32 col = ImGui::ColorConvertFloat4ToU32({ cell.fill_color.r, cell.fill_color.g, cell.fill_color.b, cell.fill_color.a });

    for (int i = 0; i < 6; i++) {
        ImVec2 face_quad[4];
        for (int j = 0; j < 4; j++) {
            face_quad[j] = screen_vertices[faces[i][j]];
        }
        draw_list->AddConvexPolyFilled(face_quad, 4, col);
    }

    engine.polyline.use_color(cell.edge_color);
    engine.polyline.draw_aabb({ pos - extent }, { pos + extent });
}

glm::vec3 LevelEditor::coord_to_world(glm::ivec2 coord) {
    return glm::vec3(coord.x * cell_size + cell_size * 0.5f, 0.f, coord.y * cell_size + cell_size * 0.5f);
}

glm::ivec2 LevelEditor::world_to_coord(glm::vec3 pos) {
    return glm::ivec2(glm::floor(pos.x / cell_size), glm::floor(pos.z / cell_size));
}

void tmt::LevelEditor::on_editor_end() {}

}  // namespace tmt
