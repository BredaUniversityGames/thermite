#include "editor/editor.hpp"
#include "level_editor_diff.hpp"


void tmt::LevelEditorDiff::undo() 
{
	auto level_editor = tmt::editor.windows[Editor::Mode::SCENE].try_get<LevelEditor>();
	auto& cells = level_editor->get_cells();

	for (auto& [coord, cell] : added_placements) {
		cells.erase(coord);
    }
}

void tmt::LevelEditorDiff::redo() 
{
    auto level_editor = tmt::editor.windows[Editor::Mode::SCENE].try_get<LevelEditor>();
    auto& cells = level_editor->get_cells();

    for (auto& [coord, cell] : added_placements) {
        cells.emplace(coord, cell);
    }
}

void tmt::LevelEditorDiff::inspect() {}

void tmt::LevelEditorDiff::commit(const std::string& message) 
{
	if(added_placements.empty()) return;

	send_to_manager(std::move(*this), message);
}