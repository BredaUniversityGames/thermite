#include "asset_browser.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include <magic_enum/magic_enum.hpp>

namespace {

template <typename ItemType, typename ContainerType>
void apply_requests(ImGuiMultiSelectIO* io, std::vector<ItemType>& selection, const ContainerType& items) {
    for (const auto& request : io->Requests) {
        switch (request.Type) {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.clear();
                if (request.Selected) selection.insert(selection.begin(), std::begin(items), std::end(items));
                break;

            case ImGuiSelectionRequestType_SetRange: {
                if (request.Selected) {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.push_back(items.at(i));
                    }
                } else {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.erase(std::ranges::find(selection, items.at(i)));
                    }
                }
            } break;
        }
    }
}

[[nodiscard]] bool path_valid(const std::filesystem::path& path) { return !path.empty() && exists(path); }

}  // namespace

namespace tmt {

void AssetBrowser::display() {
    if (ImGui::SmallButton(ICON_MS_ARROW_BACK))
        ;

    ImGui::SameLine();

    if (ImGui::SmallButton(ICON_MS_ARROW_FORWARD))
        ;

    ImGui::SameLine();

    const std::filesystem::path parent_path = viewing_dir.parent_path();
    ImGui::BeginDisabled(!path_valid(parent_path));
    if (ImGui::SmallButton(ICON_MS_ARROW_UPWARD_ALT)) pending_viewing_dir = parent_path;
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::Text("Directory:");

    ImGui::SameLine();

    display_directory_bar();

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
    if (pending_viewing_dir == viewing_dir) pending_viewing_dir.clear();
    if (pending_viewing_dir.empty()) return;

    if (pending_viewing_dir.filename().empty()) pending_viewing_dir = pending_viewing_dir.parent_path();

    if (!exists(pending_viewing_dir)) {
        Log::warn(Log::Scope::EDITOR, "AssetBrowser trying to switch to invalid directory: {}", pending_viewing_dir.generic_string());
        pending_viewing_dir.clear();
        return;
    }

    viewing_dir = pending_viewing_dir;
    pending_viewing_dir.clear();

    selected_paths.clear();

    viewing_paths.clear();
    const std::filesystem::directory_iterator directory_iterator {viewing_dir};
    for (const std::filesystem::directory_entry& entry : directory_iterator) {
        viewing_paths.push_back(entry.path());
    }

    std::ranges::sort(viewing_paths, [](const std::filesystem::path& a, const std::filesystem::path& b) {
        if (is_directory(a) == is_directory(b)) return a < b;

        return is_directory(a);
    });
}

void AssetBrowser::display_directory_bar() {
    static std::string viewing_dir_name;

    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InputText("##ViewingDirEdit", &viewing_dir_name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft)) {
        std::filesystem::path new_dir {viewing_dir_name};

        if (!is_directory(new_dir)) new_dir = new_dir.parent_path();
        if (path_valid(new_dir)) pending_viewing_dir = new_dir;

        viewing_dir_name.clear();
    }

    if (ImGui::IsItemActive()) return;
    if (ImGui::IsItemDeactivated()) viewing_dir_name.clear();

    const ImGuiID viewing_dir_edit_id = ImGui::GetItemID();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InvisibleButton("##InputSelect", ImGui::GetItemRectSize())) {
        viewing_dir_name = viewing_dir.generic_string();

        ImGui::ActivateItemByID(viewing_dir_edit_id);
    }

    ImGui::SameLine();

    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
    std::filesystem::path intermediate_dir;
    for (const std::filesystem::path& sub_directory : viewing_dir) {
        intermediate_dir /= sub_directory;

        // Adjust to get the buttons in the middle of the InputText().
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);

        if (ImGui::SmallButton(sub_directory.generic_string().c_str())) pending_viewing_dir = intermediate_dir;
        ImGui::SameLine();

        if (viewing_dir != intermediate_dir) {
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);
            ImGui::Text(">");
            ImGui::SameLine();
        }
    }

    ImGui::NewLine();
}

void AssetBrowser::display_bookmarks() {
    constexpr ImGuiTreeNodeFlags node_flags =
        ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    constexpr ImGuiTreeNodeFlags leaf_node_flags = ImGuiTreeNodeFlags_Leaf;

    constexpr std::array bookmark_paths = magic_enum::enum_entries<IO::Location>();

    for (const auto& [location, name] : bookmark_paths) {
        const std::filesystem::path relative_path = IO::FileLocation {location, ""}.get_relative_path().parent_path();
        ImGui::TreeNodeEx(relative_path.stem().generic_string().c_str(), node_flags | leaf_node_flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        if (ImGui::IsItemClicked()) pending_viewing_dir = relative_path;
    }
}

void AssetBrowser::display_files() {
    if (!path_valid(viewing_dir)) return;

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
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (path_is_directory) pending_viewing_dir = path;
        }
        ++i;
    }

    select_io = ImGui::EndMultiSelect();
    apply_requests(select_io, selected_paths, viewing_paths);
}
}  // namespace tmt