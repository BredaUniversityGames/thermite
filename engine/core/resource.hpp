#pragma once
#include "engine/core/io.hpp"

namespace tmt {
class Resource {
   public:
    Resource(const IO::FileLocation& file_location);
    virtual ~Resource() = default;

    virtual void load() = 0;
    virtual void unload() = 0;
    virtual void reload() = 0;

    const IO::FileLocation file_location;
};

class ExampleResource : public Resource {
   public:
    ExampleResource(const IO::FileLocation& location, float arg);

   private:
    // Inherited via Resource
    void load() override;
    void unload() override;
    void reload() override;
};
}  // namespace tmt