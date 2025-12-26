#pragma once

#include "editor/core/window.hpp"
#include "engine/core/io.hpp"

#include <stack>
#include <imgui.h>

namespace tmt {

class AssetBrowser : public IWindow {
   public:
    struct Directory {
        IO::FileLocation location;
        std::vector<Directory> sub_directories;
    };

    constexpr std::string get_title() const override { return ICON_MS_FOLDER " Asset Browser"; }
    constexpr int get_window_flags() const override {return ImGuiWindowFlags_MenuBar; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override;
    void on_editor_end() override {}

   private:
    std::optional<IO::FileLocation> pending_viewing_location {viewing_location};
    IO::FileLocation viewing_location {};
    std::vector<IO::FileLocation> viewing_locations;
    std::vector<IO::FileLocation> selected_locations;

    std::stack<IO::FileLocation> undo_viewing_locations;
    std::stack<IO::FileLocation> redo_viewing_locations;

    std::vector<Directory> default_bookmarked_dirs;
    std::vector<Directory> bookmarked_dirs;

    void update_viewing_locations();
    void update_bookmark_directories();

    void move_location_stacks(std::stack<IO::FileLocation>& from, std::stack<IO::FileLocation>& to);
    void recurse_display_bookmark_dirs(const Directory& directory);

    void display_directory_bar();
    void display_bookmarks();
    void display_files();
};
}  // namespace tmt