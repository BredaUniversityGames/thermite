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

    std::vector<EventDescription*> get_event_descriptions() const;

   private:
    bool is_master;
    Bank* bank {nullptr};
    Bank* string_bank {nullptr};
};

}  // namespace tmt