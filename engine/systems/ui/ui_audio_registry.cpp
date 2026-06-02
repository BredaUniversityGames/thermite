#include "ui_audio_registry.hpp"

void tmt::UiAudioRegistry::load() {
    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "audio/menu_sounds.json" });

    if (file_data.empty()) return;

    Serializer::deserialize(tmt::json::parse(file_data), *this);
}

void tmt::UiAudioRegistry::save() const {
    tmt::json j = Serializer::serialize(*this);

    IO::write_text_file({ IO::Location::PROJECT, "audio/menu_sounds.json" }, j.dump(4));
}
