#include "audio_bank.hpp"

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/io.hpp"
#include "core/audio.hpp"

#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace tmt {

bool AudioBank::load() {
    std::vector bank_data = IO::read_file(file_location);
    if (bank_data.empty()) return false;

    bank = engine.audio.init_bank(bank_data);
    if (bank == nullptr) return false;

#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (is_master) {
        IO::FileLocation string_bank_location = file_location;
        string_bank_location.relative_path.replace_extension(".strings.bank");

        bank_data = IO::read_file(string_bank_location);
        if (bank_data.empty()) {
            Log::warn(Log::Scope::ENGINE, "Failed to load the Master.strings.bank, audio events/parameters won't have names.");
        } else {
            string_bank = engine.audio.init_bank(bank_data);
        }
    }
#endif

    return true;
}

void AudioBank::unload() {
    if (string_bank != nullptr) string_bank->unload();

    bank->unload();
}

std::vector<EventDescription*> AudioBank::get_event_descriptions() const {
    int description_count;
    FMOD_RESULT result = bank->getEventCount(&description_count);
    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get event description count from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    std::vector<EventDescription*> descriptions(description_count);
    result = bank->getEventList(descriptions.data(), description_count, nullptr);
    if (result != FMOD_OK) {
        Log::error(Log::Scope::ENGINE, "FMOD, Failed to get event descriptions from bank resource: {}", FMOD_ErrorString(result));
        return {};
    }

    return descriptions;
}

}  // namespace tmt