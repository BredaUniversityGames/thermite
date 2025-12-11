#include "audio_bank.hpp"

#include "core/logger.hpp"
#include "engine.hpp"
#include "core/io.hpp"
#include "core/audio.hpp"

namespace tmt {

bool AudioBank::load() {
    std::vector bank_data = IO::read_file(file_location);
    if (bank_data.empty()) return false;

    bank = engine.audio.load_bank(bank_data);
    if (bank == nullptr) return false;

#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    if (is_master) {
        IO::FileLocation string_bank_location = file_location;
        string_bank_location.relative_path.replace_extension(".strings.bank");

        bank_data = IO::read_file(string_bank_location);
        if (bank_data.empty()) {
            Log::warn("Failed to load the Master.strings.bank, audio events/parameters won't have names.");
        } else {
            string_bank = engine.audio.load_bank(bank_data);
        }
    }
#endif

    return true;
}

void AudioBank::unload() {}
}  // namespace tmt