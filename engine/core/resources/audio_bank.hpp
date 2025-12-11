#pragma once

#include "engine/core/resource.hpp"

namespace FMOD {
namespace Studio {
class Bank;
}
}  // namespace FMOD

namespace tmt {

// FMOD .bank audio resource.
class AudioBank : public FileResource {
   public:
    AudioBank(tmt::IO::FileLocation file_location, const bool is_master = false) : FileResource(std::move(file_location)), is_master(is_master) {}

    bool load() override;
    void unload() override;

   private:
    bool is_master;
    FMOD::Studio::Bank* bank {nullptr};
    FMOD::Studio::Bank* string_bank {nullptr};
};

}  // namespace tmt