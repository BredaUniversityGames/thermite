#pragma once

#include "editor/core/window.hpp"

#include <filesystem>
#include <stack>

namespace tmt {

class AssetBrowser : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_FOLDER " Asset Browser"; }

    // Inherited via IWindow.
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override;
    void on_editor_end() override {}

   private:
    std::filesystem::path pending_viewing_dir;
    std::filesystem::path viewing_dir;
    std::vector<std::filesystem::path> viewing_paths;
    std::vector<std::filesystem::path> selected_paths;

    std::stack<std::filesystem::path> undo_viewing_dirs;
    std::stack<std::filesystem::path> redo_viewing_dirs;

    void update_viewing_paths();

    void display_directory_bar();
    void display_bookmarks();
    void display_files();
};
}  // namespace tmt