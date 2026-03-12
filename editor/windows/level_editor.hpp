#pragma once
#include "editor/core/window.hpp"

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

   private:

    struct ViewportContext {
        float width;
        float height;
        glm::vec2 p;
        glm::mat4 view_project;
    } viewport_context;

    struct Cell {
        glm::vec4 fill_color;
        glm::vec4 edge_color;
    };

    void draw_cell(ImDrawList* draw_list, glm::vec3 pos, const Cell& cell);
    glm::vec3 coord_to_world(glm::ivec2 coord);
    glm::ivec2 world_to_coord(glm::vec3 coord);

    glm::vec3 place_xz_pos;
    float highlight_timer = 0.f;
    std::vector<glm::ivec2> placing_coords;


    bool enable_editing = false;
    bool mirror_x = false;
    float cell_size;
    std::unordered_map<glm::ivec2, Cell> cells;
};

}  // namespace tmt