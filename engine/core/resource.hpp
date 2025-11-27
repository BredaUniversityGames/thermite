#pragma once
#include "engine/core/io.hpp"

namespace tmt {
class Resource {
   protected:
    Resource(const IO::FileLocation& file_location);
   public:
    virtual ~Resource() = default;

    virtual void load() = 0;
    virtual void unload() = 0;
    virtual void reload() = 0;

    const IO::FileLocation file_location;
};

class ExampleResource : public Resource {
   public:
    ExampleResource(/*const IO::FileLocation& location, */float example_arg);

    // Inherited via Resource
    void load() override;
    void unload() override;
    void reload() override;

    float important_float;

   private:
};
}  // namespace tmt