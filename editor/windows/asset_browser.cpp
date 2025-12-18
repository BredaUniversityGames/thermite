#include "asset_browser.hpp"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

namespace {

template <typename ItemType, typename ContainerType>
    void apply_requests(ImGuiMultiSelectIO* io, std::vector<ItemType>& selection, const ContainerType& items)
    {
        for (const auto& request : io->Requests)
        {
            switch (request.Type)
            {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.clear();
                if (request.Selected) selection.insert(selection.begin(), std::begin(items), std::end(items));
                break;

            case ImGuiSelectionRequestType_SetRange: {
                if (request.Selected)
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.push_back(items.at(i));
                    }
                }
                else
                {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++)
                    {
                        selection.erase(std::ranges::find(selection, items.at(i)));
                    }
                }
            }
            break;
            }
        }
    }

}  // namespace

// namespace ImGui {
//
// namespace {
//
// ImGuiWindow* GetCombinedRootWindow(ImGuiWindow* window, bool popup_hierarchy, bool dock_hierarchy) {
//     const ImGuiWindow* last_window = nullptr;
//     while (last_window != window) {
//         last_window = window;
//         window = window->RootWindow;
//         if (popup_hierarchy) window = window->RootWindowPopupTree;
//         if (dock_hierarchy) window = window->RootWindowDockTree;
//     }
//     return window;
// }
//
// bool IsWindowFocused(ImGuiWindow* window, ImGuiFocusedFlags flags = 0) {
//     ImGuiContext& g = *GImGui;
//     ImGuiWindow* ref_window = g.NavWindow;
//
//     if (ref_window == nullptr) return false;
//     if (flags & ImGuiFocusedFlags_AnyWindow) return true;
//
//     IM_ASSERT(window);  // Not inside a Begin()/End()
//     const bool popup_hierarchy = (flags & ImGuiFocusedFlags_NoPopupHierarchy) == 0;
//     const bool dock_hierarchy = (flags & ImGuiFocusedFlags_DockHierarchy) != 0;
//     if (flags & ImGuiHoveredFlags_RootWindow) window = GetCombinedRootWindow(window, popup_hierarchy, dock_hierarchy);
//
//     if (flags & ImGuiHoveredFlags_ChildWindows)
//         return IsWindowChildOf(ref_window, window, popup_hierarchy, dock_hierarchy);
//     else
//         return (ref_window == window);
// }
//
// bool IsWindowFocused(const char* name, ImGuiFocusedFlags flags = 0) {
//     auto* window = FindWindowByName(name);
//     if (window == nullptr) return false;
//     return IsWindowFocused(window, flags);
// }
//
// }  // namespace
//
// }  // namespace ImGui

namespace tmt {
void AssetBrowser::display() {
    const float available_width = ImGui::GetContentRegionAvail().x;
    if (!ImGui::BeginTable("AssetBrowserTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) return;

    ImGui::TableSetupColumn("###Default folders", ImGuiTableColumnFlags_WidthFixed, available_width / 6.0f);

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Bookmarks")) {
        display_bookmarks();
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Files")) {
        display_files();
    }
    ImGui::EndChild();

    ImGui::EndTable();
}

void AssetBrowser::on_editor_update(const FrameData&) { update_viewing_paths(); }

void AssetBrowser::update_viewing_paths() {
    if (pending_viewing_directory.empty() || !exists(pending_viewing_directory)) return;

    viewing_directory = pending_viewing_directory;

    viewing_paths.clear();
    const std::filesystem::directory_iterator directory_iterator {viewing_directory};
    for (const auto& entry : directory_iterator) {
        viewing_paths.push_back(entry.path());
    }

    std::ranges::sort(viewing_paths, [](const std::filesystem::path& a, const std::filesystem::path& b) {
        if (is_directory(a) == is_directory(b)) return a < b;

        return is_directory(a);
    });
}

void AssetBrowser::display_bookmarks() {
    constexpr ImGuiTreeNodeFlags node_flags =
        ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    constexpr ImGuiTreeNodeFlags leaf_node_flags = ImGuiTreeNodeFlags_Leaf;

    constexpr std::array bookmark_paths = magic_enum::enum_entries<IO::Location>();

    for (const auto& [location, name] : bookmark_paths) {
        const std::filesystem::path relative_path = IO::FileLocation {location, ""}.get_relative_path().parent_path();
        ImGui::TreeNodeEx(relative_path.stem().generic_string().c_str(), node_flags | leaf_node_flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        if (ImGui::IsItemClicked()) pending_viewing_directory = relative_path;
    }
}

void AssetBrowser::display_files() {
    if (viewing_directory.empty() || !std::filesystem::exists(viewing_directory)) return;

    constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect2d | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                            ImGuiMultiSelectFlags_ScopeWindow | ImGuiMultiSelectFlags_SelectOnClickRelease;

    ImGuiMultiSelectIO* select_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected_paths.size()), static_cast<int>(viewing_paths.size()));
    apply_requests(select_io, selected_paths, viewing_paths);

    ImS64 i = 0;
    for (const std::filesystem::path& path : viewing_paths) {
        const bool path_is_directory = is_symlink(path) || is_directory(path);
        const std::string path_name = (path_is_directory ? ICON_MS_FOLDER " " : ICON_MS_DESCRIPTION " ") + path.filename().generic_string();

        const bool is_selected = std::ranges::find(selected_paths, path) != selected_paths.end();

        ImGui::SetNextItemSelectionUserData(i);
        ImGui::Selectable(path_name.c_str(), is_selected);
        if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (path_is_directory) pending_viewing_directory = path;
        }
        ++i;
    }

    select_io = ImGui::EndMultiSelect();
    apply_requests(select_io, selected_paths, viewing_paths);
}
}  // namespace tmt