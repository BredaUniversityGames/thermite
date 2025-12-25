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

void recurse_parse_directory(tmt::AssetBrowser::Directory& directory) {
    std::filesystem::directory_iterator iterator {directory.location.get_relative_path()};
    for (const auto& entry : iterator) {
        if (!entry.is_directory()) continue;

        tmt::AssetBrowser::Directory& sub_directory = directory.sub_directories.emplace_back();
        sub_directory.location = {directory.location.sub_location, directory.location.relative_path / entry.path().filename()};
        recurse_parse_directory(sub_directory);
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

    const std::filesystem::path& current_path = viewing_location.relative_path;
    ImGui::BeginDisabled(current_path.empty());
    if (ImGui::SmallButton(ICON_MS_ARROW_UPWARD_ALT)) pending_viewing_location = {viewing_location.sub_location, current_path.parent_path()};
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::Text("Directory:");

    ImGui::SameLine();

    display_directory_bar();
    // ImGui::SameLine();

    const auto& bookmark_iterator = std::ranges::find_if(bookmarked_dirs, [this](const Directory& dir) { return dir.location == viewing_location; });
    ImGui::BeginDisabled(viewing_location.relative_path.empty() || bookmark_iterator != bookmarked_dirs.end());
    if (ImGui::SmallButton(ICON_MS_BOOKMARK)) bookmarked_dirs.emplace_back(viewing_location);
    ImGui::EndDisabled();

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

void AssetBrowser::on_editor_start() {
    constexpr std::array bookmark_paths = magic_enum::enum_entries<IO::Location>();
    for (const auto& [location, name] : bookmark_paths) {
        default_bookmarked_dirs.emplace_back(IO::FileLocation {location, ""});
    }
}

void AssetBrowser::on_editor_update(const FrameData&) {
    update_viewing_locations();

    update_bookmark_directories();
}

void AssetBrowser::update_viewing_locations() {
    if (!pending_viewing_location.has_value()) return;

    const std::filesystem::path& path = pending_viewing_location->relative_path;
    if (!path.empty() && path.filename().empty()) pending_viewing_location->relative_path = path.parent_path();

    if (!exists(pending_viewing_location->get_relative_path())) {
        Log::warn(Log::Scope::EDITOR, "AssetBrowser trying to switch to invalid directory: {}", pending_viewing_location->get_relative_path().generic_string());
        pending_viewing_location.reset();
        return;
    }

    viewing_location = *pending_viewing_location;
    pending_viewing_location.reset();

    selected_locations.clear();
    viewing_locations.clear();

    const std::filesystem::directory_iterator directory_iterator {viewing_location.get_relative_path()};
    for (const std::filesystem::directory_entry& entry : directory_iterator) {
        viewing_locations.emplace_back(viewing_location.sub_location, relative(entry.path(), IO::get_sub_location_path(viewing_location.sub_location)));
    }

    // Sort the paths by name but make sure that directories are always first (just like how file explorer does it).
    std::ranges::sort(viewing_locations, [](const IO::FileLocation& a, const IO::FileLocation& b) {
        const std::filesystem::path& a_path = a.get_relative_path();
        const std::filesystem::path& b_path = b.get_relative_path();

        if (is_directory(a_path) == is_directory(b_path)) return a_path < b_path;
        return is_directory(a_path);
    });
}

void AssetBrowser::update_bookmark_directories() {
    // Update the default bookmarks (asset dirs for: engine, editor and project).
    for (auto& directory : default_bookmarked_dirs) {
        directory.sub_directories.clear();
        recurse_parse_directory(directory);
    }

    // Update the user added bookmarks.
    for (auto& directory : bookmarked_dirs) {
        directory.sub_directories.clear();
        recurse_parse_directory(directory);
    }
}

void AssetBrowser::display_directory_bar() {
    static std::string viewing_dir_name;

    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InputText("##ViewingDirEdit", &viewing_dir_name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft)) {
        IO::FileLocation new_location {viewing_location.sub_location, viewing_dir_name};

        if (!is_directory(new_location.get_relative_path())) new_location.relative_path = new_location.relative_path.parent_path();
        if (path_valid(new_location.get_relative_path())) pending_viewing_location = new_location;

        viewing_dir_name.clear();
    }

    if (ImGui::IsItemActive()) return;
    if (ImGui::IsItemDeactivated()) viewing_dir_name.clear();

    const ImGuiID viewing_dir_edit_id = ImGui::GetItemID();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InvisibleButton("##InputSelect", ImGui::GetItemRectSize())) {
        viewing_dir_name = viewing_location.relative_path.generic_string();

        ImGui::ActivateItemByID(viewing_dir_edit_id);
    }

    ImGui::SameLine();

    // Adjust to get the text in the middle of the InputText().
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin() + ImGui::GetStyle().FramePadding);

    // Add the sub_path button which has to be handled separately.
    const std::string sub_path_name = IO::get_sub_location_path(viewing_location.sub_location).stem().generic_string();
    if (ImGui::SmallButton(sub_path_name.c_str())) pending_viewing_location = {viewing_location.sub_location, ""};

    ImGui::SameLine();

    // Add the ">" for the sub_path button.
    const bool valid_relative_path = !viewing_location.relative_path.empty();
    if (valid_relative_path) {
        // Adjust to get the text in the middle of the InputText().
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);
        ImGui::Text(">");
    }

    ImGui::SameLine();

    std::filesystem::path intermediate_dir;
    for (const std::filesystem::path& sub_directory : viewing_location.relative_path) {
        intermediate_dir /= sub_directory;

        // Adjust to get the buttons in the middle of the InputText().
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);

        if (ImGui::SmallButton(sub_directory.generic_string().c_str())) pending_viewing_location = {viewing_location.sub_location, intermediate_dir};
        ImGui::SameLine();

        if (viewing_location.relative_path != intermediate_dir) {
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().FramePadding);
            ImGui::Text(">");
            ImGui::SameLine();
        }
    }

    ImGui::NewLine();
}

void AssetBrowser::recurse_display_bookmark_dirs(const Directory& directory) {
    constexpr ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    constexpr ImGuiTreeNodeFlags leaf_node_flags = ImGuiTreeNodeFlags_Leaf;

    const std::string& directory_path = ICON_MS_FOLDER_OPEN " " + directory.location.get_relative_path().filename().generic_string();
    const bool node_open = ImGui::TreeNodeEx(directory_path.c_str(), node_flags | (directory.sub_directories.empty() ? leaf_node_flags : 0));

    if (ImGui::IsItemClicked()) pending_viewing_location = directory.location;
    if (node_open) {
        for (const auto& sub_directory : directory.sub_directories) {
            recurse_display_bookmark_dirs(sub_directory);
        }

        ImGui::TreePop();
    }
}

void AssetBrowser::display_bookmarks() {
    for (const auto& directory : default_bookmarked_dirs) {
        recurse_display_bookmark_dirs(directory);
    }

    ImGui::SeparatorText("Bookmarks");

    for (const auto& directory : bookmarked_dirs) {
        const std::string absolute_path_string = absolute(directory.location.get_relative_path()).generic_string();

        ImGui::PushID(absolute_path_string.c_str());
        recurse_display_bookmark_dirs(directory);
        ImGui::PopID();
    }
}

void AssetBrowser::display_files() {
    if (!path_valid(viewing_location.get_relative_path())) return;

    constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect2d | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                            ImGuiMultiSelectFlags_ScopeWindow | ImGuiMultiSelectFlags_SelectOnClickRelease;

    ImGuiMultiSelectIO* select_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected_locations.size()), static_cast<int>(viewing_locations.size()));
    apply_requests(select_io, selected_locations, viewing_locations);

    ImS64 i = 0;
    for (const IO::FileLocation& location : viewing_locations) {
        const bool location_is_directory = is_directory(location.get_relative_path());
        const std::string location_name = (location_is_directory ? ICON_MS_FOLDER " " : ICON_MS_DESCRIPTION " ") + location.relative_path.filename().generic_string();

        const bool is_selected = std::ranges::find(selected_locations, location) != selected_locations.end();

        ImGui::SetNextItemSelectionUserData(i);
        ImGui::Selectable(location_name.c_str(), is_selected);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (location_is_directory) {
                pending_viewing_location = location;
            } else {
                const std::string open_file_command = std::format(R"(start "" "{}")", location.get_relative_path().generic_string());
                if (system(open_file_command.c_str()) != 0) Log::warn(Log::Scope::ENGINE, "Couldn't open file.");
            }
        }
        ++i;
    }

    select_io = ImGui::EndMultiSelect();
    apply_requests(select_io, selected_locations, viewing_locations);
}
}  // namespace tmt