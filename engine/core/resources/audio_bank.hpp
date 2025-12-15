#pragma once

#include "core/resource.hpp"
#include "core/audio.hpp"

namespace FMOD {
namespace Studio {
class Bank;
}
}  // namespace FMOD

namespace tmt {

// FMOD .bank audio resource.
class AudioBank : public FileResource {
   public:
    AudioBank(IO::FileLocation file_location, const bool is_master = false) : FileResource(std::move(file_location)), is_master(is_master) {}

    bool load() override;
    void unload() override;

    std::vector<AudioEvent> get_audio_events() const;
    std::vector<VolumeControl> get_volume_controls() const;

   private:
    bool is_master;
    FMOD::Studio::Bank* bank {nullptr};
    FMOD::Studio::Bank* string_bank {nullptr};
};

}  // namespace tmt