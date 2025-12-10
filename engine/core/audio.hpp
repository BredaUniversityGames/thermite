#pragma once

#include <set>

#include "fmod_common.h"

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

class Audio {
   public:
    Audio() = default;
    ~Audio();

    void Init();
    void update();

    // Load an FMOD bank into the system to initialize all the sounds.
    FMOD::Studio::Bank* load_bank(const std::vector<char>& data);

    /* Audio event functions. */

    FMOD::Studio::EventInstance* start_event(const FMOD::Studio::EventDescription* event_description);
    [[nodiscard]] bool is_event_instance_valid(FMOD::Studio::EventInstance* instance) const;
    void stop_all_event_instances();

    /* VCA (aka, volume group) functions. */

    [[nodiscard]] FMOD::Studio::VCA* get_vca(const FMOD_GUID& guid) const;
    [[nodiscard]] static float get_vca_volume(const FMOD::Studio::VCA* vca);
    static void set_vca_volume(FMOD::Studio::VCA* vca, float volume);

   private:
    FMOD::Studio::System* system;
    FMOD::System* core_system;

    std::set<FMOD::Studio::EventInstance*> active_instances;
};

}  // namespace tmt