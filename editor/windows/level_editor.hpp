#pragma once
#include "editor/core/window.hpp"
#include "engine/tools/poisson_disc_walk.hpp"
#include "engine/tools/types/color.hpp"
struct ImDrawList;

namespace std {

template <>
struct hash<glm::ivec2> {
    std::size_t operator()(const glm::ivec2& v) const noexcept {
        std::size_t h1 = std::hash<int> {}(v.x);
        std::size_t h2 = std::hash<int> {}(v.y);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

}  // namespace std

namespace tmt {

class LevelEditor : public IWindow {
   public:
    // Inherited via IWindow
    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    constexpr std::string get_title() const override { return "Level Editor"; };

    struct Cell {
        int template_index;
    };
    struct CellTemplate
    {
        int seed = 42;
        bool random_seed;
        std::string name;

        float height ;
        PoissonField field;
        RGBA color;
    };

    std::unordered_map<glm::ivec2, Cell>& get_cells() {return cells;}
   private:

    struct ViewportContext {
        float width;
        float height;
        glm::vec2 p;
        glm::mat4 view_project;
    } viewport_context;

    struct LevelCellGrouperTag {};

    std::vector<CellTemplate> pickable_cell_templates;
    int selected_template = -1;

    enum class Brush
    {
        PLACE,
        REMOVE,
        REPLACE,
        MAX
    };

    int selected_brush_mode = 0;

    bool window_displayed = false;

    void draw_cell(ImDrawList* draw_list, glm::vec3 pos, const Cell& cell, glm::vec4 col = glm::vec4(0.f));
    glm::vec3 coord_to_world(glm::ivec2 coord);
    glm::ivec2 world_to_coord(glm::vec3 coord);

    glm::vec3 place_xz_pos;
    float highlight_timer = 0.f;
    std::vector<glm::ivec2> placing_coords;

    //user editable
    bool enable_editing = false;
    bool mirror_x = false;
    float cell_size;
    glm::vec3 global_field_offset;
    std::unordered_map<glm::ivec2, Cell> cells;
};

}  // namespace tmt
TMT_OBJECT(tmt::LevelEditor::CellTemplate, (name, seed, height, field, color));