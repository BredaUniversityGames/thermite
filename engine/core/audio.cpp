#include "audio.hpp"

#include "core/logger.hpp"

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

FMOD::Studio::Bank* Audio::init_bank(const std::vector<char>& bank_data) const {
    FMOD::Studio::Bank* bank = nullptr;
    FMOD_RESULT result = system->loadBankMemory(bank_data.data(), static_cast<int>(bank_data.size()), FMOD_STUDIO_LOAD_MEMORY, FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    if (TryLogError(result, "Bank could not be initialized from memory")) return nullptr;

    // Load all the bank's sample data immediately.
    result = bank->loadSampleData();
    if (TryLogError(result, "Bank could not load sample data")) return nullptr;

    // Enable this to wait for loading to finish.
    result = system->flushSampleLoading();
    if (TryLogError(result, "Bank could not flush sample loading")) return nullptr;

    return bank;
}

FMOD::Studio::EventDescription* Audio::get_event(const FMOD_GUID& guid) const {
    FMOD::Studio::EventDescription* description;
    system->getEventByID(&guid, &description);

    return description;
}

std::string Audio::get_event_path([[maybe_unused]] const FMOD::Studio::EventDescription* description) {
#if defined(THERMITE_EDITOR) || defined(THERMITE_DEBUG)
    // This function only returns useful paths in the editor or in debug mode, since these aren't necessary for audio to function, and it allows us to skip loading string banks in release
    // game.
    std::string name;
    name.resize(512);

    int size = 0;
    const FMOD_RESULT result = description->getPath(name.data(), static_cast<int>(name.size()), &size);
    name.resize(size);

    TryLogError(result, "Failed to get the name of event");

    return name;
#else
    return "";
#endif
}

FMOD::Studio::EventInstance* Audio::play_event(const FMOD::Studio::EventDescription* description) {
    if (TryLogError(description == nullptr, "Invalid EventDescription pointer")) return nullptr;

    // Play the event, creating an instance.
    FMOD::Studio::EventInstance* event_instance = nullptr;
    FMOD_RESULT result = description->createInstance(&event_instance);
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

void Audio::set_global_parameter(const FMOD_STUDIO_PARAMETER_ID& id, const float value) const {
    const FMOD_RESULT result = system->setParameterByID(id, value);

    TryLogError(result, "Failed to set parameter");
}

void Audio::set_global_parameter(const FMOD_STUDIO_PARAMETER_ID& id, const std::string& value) const {
    const FMOD_RESULT result = system->setParameterByIDWithLabel(id, value.c_str());

    TryLogError(result, "Failed to set parameter with label");
}

FMOD::Studio::VCA* Audio::get_vca(const FMOD_GUID& guid) const {
    FMOD::Studio::VCA* vca;
    const FMOD_RESULT result = system->getVCAByID(&guid, &vca);

    if (TryLogError(result, "Failed to get VCA")) return nullptr;

    return vca;
}

float Audio::get_vca_volume(const FMOD::Studio::VCA* vca) {
    if (TryLogError(vca == nullptr, "Invalid VCA pointer")) return 0.0f;

    float volume = 0.0f;
    const FMOD_RESULT result = vca->getVolume(&volume);
    if (TryLogError(result, "Failed to get VCA volume")) return 0.0f;

    return volume;
}

void Audio::set_vca_volume(FMOD::Studio::VCA* vca, const float volume) {
    if (TryLogError(vca == nullptr, "Invalid VCA pointer")) return;

    FMOD_RESULT result = vca->setVolume(volume);
    TryLogError(result, "Failed to set VCA volume");
}

}  // namespace tmt