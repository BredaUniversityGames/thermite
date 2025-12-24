#pragma once

#include "editor/core/window.hpp"
#include "engine/core/io.hpp"

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
    std::optional<IO::FileLocation> pending_viewing_location {viewing_location};
    IO::FileLocation viewing_location {};
    std::vector<IO::FileLocation> viewing_locations;
    std::vector<IO::FileLocation> selected_locations;

    std::stack<IO::FileLocation> undo_viewing_locations;
    std::stack<IO::FileLocation> redo_viewing_locations;

    void update_viewing_paths();

    void display_directory_bar();
    void display_bookmarks();
    void display_files();
};
}  // namespace tmt