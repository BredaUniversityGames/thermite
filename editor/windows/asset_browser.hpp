#pragma once

#include "editor/core/window.hpp"

#include <filesystem>

namespace tmt {

struct PathInfo {
    std::filesystem::path path;
    std::string display_name;
};

class AssetBrowser : public IWindow {
   public:
    std::string get_title() const override { return ICON_MS_FOLDER " Asset Browser"; }

    // Inherited via IWindow.
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override;
    void on_editor_end() override {}

   private:
    std::filesystem::path viewing_directory;
    std::vector<std::filesystem::path> viewing_paths;

    std::vector<std::filesystem::path> selected_paths;

    std::filesystem::path pending_viewing_directory;
    void update_viewing_paths();

    void display_bookmarks();
    void display_files();
};
}  // namespace tmt