#include "resource.hpp"
#include "resource.hpp"

namespace tmt {
Resource::Resource(const IO::FileLocation& _file_location) : file_location(_file_location) {}
ExampleResource::ExampleResource(const IO::FileLocation& location, float arg) : Resource(location) {}
void ExampleResource::load() {}
void ExampleResource::unload() {}
void ExampleResource::reload() {}
}  // namespace tmt
