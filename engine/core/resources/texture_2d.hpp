#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/resource.hpp"

namespace tmt {

/* 2D Texture Resource - used for UI and Particles */
class Texture2D : public FileResource {
   public:
    Texture2D(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    Texture texture {};
    Image image {};
};

}  // namespace tmt