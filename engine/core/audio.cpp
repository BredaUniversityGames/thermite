#include "audio.hpp"

#include "core/logger.hpp"

#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <fmod_errors.h>

namespace tmt {

namespace {
// Logs the message as an error if the result is not FMOD_OK.
bool TryLogError(const FMOD_RESULT result, const spdlog::string_view_t message) {
    if (result) {
        Log::error(Log::Scope::ENGINE, "FMOD, {}: {}", message, FMOD_ErrorString(result));
        return true;
    }

    return false;
}

// Logs the message as an error if fail is false (used with error check e.g. "pointer == nullptr").
bool TryLogError(const bool fail, const spdlog::string_view_t message) {
    if (fail) Log::error(Log::Scope::ENGINE, "FMOD, {}", message);
    return fail;
}
}  // namespace

Audio::~Audio() { system->release(); }

void Audio::Init() {
    Log::info("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.");

    FMOD_RESULT result = FMOD::Studio::System::create(&system);
    if (TryLogError(result, "Failed to create the FMOD Studio System")) return;

    // Initialize FMOD Studio, which will also initialize FMOD Core.
    result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (TryLogError(result, "Failed to initialize the FMOD Studio System")) return;

    result = system->getCoreSystem(&core_system);
    TryLogError(result, "Failed to get the FMOD Studio System after initialization");
}

void Audio::update() {
    system->update();

    std::erase_if(active_instances, [](const FMOD::Studio::EventInstance* instance) {
        FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
        instance->getPlaybackState(&state);
        return state == FMOD_STUDIO_PLAYBACK_STOPPED;
    });
}

FMOD::Studio::Bank* Audio::load_bank(const std::vector<char>& data) const {
    FMOD::Studio::Bank* bank = nullptr;
    FMOD_RESULT result = system->loadBankMemory(data.data(), static_cast<int>(data.size()), FMOD_STUDIO_LOAD_MEMORY, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    if (TryLogError(result, "FMOD bank could not be loaded")) return nullptr;

    // Load all the bank's sample data immediately.
    result = bank->loadSampleData();
    if (TryLogError(result, "FMOD bank could not load sample data")) return nullptr;

    // Enable this to wait for loading to finish.
    result = system->flushSampleLoading();
    if (TryLogError(result, "FMOD bank could not flush sample loading")) return nullptr;

    return bank;
}

FMOD::Studio::EventDescription* Audio::get_event(const FMOD_GUID& guid) const {
    FMOD::Studio::EventDescription* description;
    system->getEventByID(&guid, &description);

    return description;
}

std::string Audio::get_event_path(const FMOD::Studio::EventDescription* description) {
    std::string name;
    name.resize(512);

    int size = 0;
    const FMOD_RESULT result = description->getPath(name.data(), static_cast<int>(name.size()), &size);
    name.resize(size);

#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG) 
    // Only perform this error check only in debug mode or the editor, string banks (aka FMOD files that hold names) have no purpose outside of debug mode or the editor.
    TryLogError(result, "Failed to get the name of event");
#endif

    return name;
}

FMOD::Studio::EventInstance* Audio::play_event(const FMOD::Studio::EventDescription* event_description) {
    // Play the event, creating an instance.
    FMOD::Studio::EventInstance* event_instance = nullptr;
    FMOD_RESULT result = event_description->createInstance(&event_instance);
    if (TryLogError(result, "Event instance with this description could not be created")) return nullptr;

    active_instances.emplace(event_instance);

    // trigger the event
    result = event_instance->start();
    if (TryLogError(result, "Event instance failed to start")) return nullptr;

    // mark it for release immediately
    result = event_instance->release();
    if (TryLogError(result, "Event instance failed to mark for release")) return nullptr;

    return event_instance;
}

bool Audio::is_event_instance_valid(FMOD::Studio::EventInstance* instance) const { return active_instances.contains(instance); }

void Audio::stop_all_event_instances() {
    for (FMOD::Studio::EventInstance* instance : active_instances) {
        instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
    }

    active_instances.clear();
}

FMOD::Studio::VCA* Audio::get_vca(const FMOD_GUID& guid) const {
    FMOD::Studio::VCA* vca;
    system->getVCAByID(&guid, &vca);

    return vca;
}

float Audio::get_vca_volume(const FMOD::Studio::VCA* vca) {
    if (TryLogError(vca == nullptr, "Invalid VCA pointer")) return 0.0f;

    float volume = 0.0f;
    vca->getVolume(&volume);

    return volume;
}

void Audio::set_vca_volume(FMOD::Studio::VCA* vca, const float volume) {
    if (TryLogError(vca == nullptr, "Invalid VCA pointer")) return;

    FMOD_RESULT result = vca->setVolume(volume);
    TryLogError(result, "Failed to set VCA volume");
}

}  // namespace tmt