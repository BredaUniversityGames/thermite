#include "save_data.hpp"
#include "editor/editor.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/all.hpp"

namespace tmt {

void SaveData::save() const {
    const json j = tmt::Serializer::serialize(*this);
    //for (const auto& system : editor.systems) {
    //    //system->
    //}
    IO::write_text_file(Config::FILE_LOCATION, j.dump(), true);
}

void SaveData::load() {
    const std::string file_content = IO::read_or_create_text_file(Config::FILE_LOCATION);
    if (file_content.empty()) return;
    const json j = json::parse(file_content);
    tmt::Serializer::deserialize(j, *this);
}

}  // namespace tmt