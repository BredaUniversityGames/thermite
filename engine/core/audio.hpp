#pragma once

#include <set>
#include <string>

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

using VCA = FMOD::Studio::VCA;
using Bank = FMOD::Studio::Bank;
using EventInstance = FMOD::Studio::EventInstance;
using EventDescription = FMOD::Studio::EventDescription;

class Audio {
   public:
    Audio() = default;
    ~Audio();

    void Init();
    void update();

    // Load an FMOD bank into the system to initialize all its assets.
    [[nodiscard]] Bank* load_bank(const std::vector<char>& data) const;

    /* Audio event functions. */

    static std::string get_event_path(const EventDescription* description);
    [[nodiscard]] EventDescription* get_event(const FMOD_GUID& guid) const;
    [[nodiscard]] EventInstance* play_event(const EventDescription* description);
    [[nodiscard]] bool is_event_instance_valid(EventInstance* instance) const;
    void stop_all_event_instances();

    void set_global_parameter(const FMOD_STUDIO_PARAMETER_ID& id, float value) const;
    void set_global_parameter(const FMOD_STUDIO_PARAMETER_ID& id, const std::string& value) const;

    /* VCA (aka, volume group) functions. */

    [[nodiscard]] VCA* get_vca(const FMOD_GUID& guid) const;
    [[nodiscard]] static float get_vca_volume(const VCA* vca);
    static void set_vca_volume(VCA* vca, float volume);

   private:
    FMOD::Studio::System* system;
    FMOD::System* core_system;

    std::set<EventInstance*> active_instances;
};

}  // namespace tmt