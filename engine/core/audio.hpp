#pragma once

#include <set>
#include <string>

#include "fmod_studio_common.h"

struct FMOD_GUID;
struct FMOD_STUDIO_PARAMETER_ID;

namespace FMOD {

class System;

namespace Studio {

class VCA;
class Bank;
class System;
class EventInstance;
class EventDescription;

}  // namespace Studio

}  // namespace FMOD

namespace tmt {

struct AudioParameter {
    FMOD_STUDIO_PARAMETER_ID id;
};

class AudioInstance {
   public:
    AudioInstance(FMOD::Studio::EventInstance* instance) : instance {instance} {}

    [[nodiscard]] bool is_valid() const;
    void stop(FMOD_STUDIO_STOP_MODE stop_mode = FMOD_STUDIO_STOP_IMMEDIATE) const;
    void set_parameter(const AudioParameter& event_parameter, float value) const;
    void set_parameter(const AudioParameter& event_parameter, int value) const;
    void set_label_parameter(const AudioParameter& event_parameter, const std::string& value) const;

   private:
    FMOD::Studio::EventInstance* instance {nullptr};
};

class AudioEvent {
   public:
    AudioEvent(FMOD::Studio::EventDescription* description) : description {description} {}

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] std::string get_path() const;
    AudioInstance play() const;

   private:
    FMOD::Studio::EventDescription* description {nullptr};
};

class VolumeControl {
   public:
    VolumeControl(FMOD::Studio::VCA* vca) : vca {vca} {}

    [[nodiscard]] bool is_valid() const;
    [[nodiscard]] float get_volume() const;
    void set_volume(float volume) const;

   private:
    FMOD::Studio::VCA* vca {nullptr};
};

class Audio {
    friend class AudioInstance;
    friend class AudioEvent;

   public:
    Audio() = default;
    ~Audio();

    void Init();
    void update();

    // Initialize an FMOD bank with all its assets from a .bank file in memory.
    [[nodiscard]] FMOD::Studio::Bank* init_bank(const std::vector<char>& bank_data) const;

    /* Audio event functions. */

    [[nodiscard]] AudioEvent get_event(const FMOD_GUID& guid) const;
    void stop_all_audio_instances();

    void set_global_parameter(const AudioParameter& id, float value) const;
    void set_global_parameter(const AudioParameter& id, int value) const;
    void set_global_label_parameter(const AudioParameter& id, const std::string& value) const;

    /* VCA (aka, volume group) functions. */

    [[nodiscard]] VolumeControl get_volume_control(const FMOD_GUID& guid) const;

   private:
    FMOD::Studio::System* system;
    FMOD::System* core_system;

    std::set<FMOD::Studio::EventInstance*> active_instances;
};

}  // namespace tmt