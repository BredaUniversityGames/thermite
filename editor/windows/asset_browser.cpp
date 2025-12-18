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

    const std::filesystem::path parent_path = viewing_location.relative_path;
    ImGui::BeginDisabled(!path_valid(parent_path));
    if (ImGui::SmallButton(ICON_MS_ARROW_UPWARD_ALT)) pending_viewing_location = {viewing_location.sub_location, parent_path};
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
    if (!pending_viewing_location.has_value() || pending_viewing_location == viewing_location) {
        pending_viewing_location.reset();
        return;
    }

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
        viewing_locations.emplace_back(viewing_location.sub_location, std::filesystem::relative(entry.path(), IO::get_sub_location_path(viewing_location.sub_location)));
    }

    std::ranges::sort(viewing_locations, [](const IO::FileLocation& a, const IO::FileLocation& b) {
        const std::filesystem::path& a_path = a.get_relative_path();
        const std::filesystem::path& b_path = b.get_relative_path();

        if (is_directory(a_path) == is_directory(b_path)) return a_path < b_path;
        return is_directory(a_path);
    });
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
    const bool valid_relative_path = !viewing_location.relative_path.empty();
    ImGui::Text((valid_relative_path ? "%s >" : "%s"), IO::get_sub_location_path(viewing_location.sub_location).stem().generic_string().c_str());

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

void AssetBrowser::display_bookmarks() {
    constexpr ImGuiTreeNodeFlags node_flags =
        ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    constexpr ImGuiTreeNodeFlags leaf_node_flags = ImGuiTreeNodeFlags_Leaf;

    constexpr std::array bookmark_paths = magic_enum::enum_entries<IO::Location>();
    for (const auto& [location, name] : bookmark_paths) {
        const std::filesystem::path sub_location_path = IO::get_sub_location_path(location);
        ImGui::TreeNodeEx(sub_location_path.stem().generic_string().c_str(), node_flags | leaf_node_flags | ImGuiTreeNodeFlags_NoTreePushOnOpen);

        if (ImGui::IsItemClicked()) pending_viewing_location = {location, ""};
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
            if (location_is_directory) pending_viewing_location = location;
        }
        ++i;
    }

    select_io = ImGui::EndMultiSelect();
    apply_requests(select_io, selected_locations, viewing_locations);
}
}  // namespace tmt