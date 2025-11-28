#pragma once
#include <memory>
#include "resource.hpp"
#include "logger.hpp"

namespace tmt {

template <typename T>
concept ResourceType = std::derived_from<T, Resource>;

class Resources {
   public:
    template <ResourceType T, typename... Args>
    requires std::constructible_from<T, const IO::FileLocation&, Args...>
    std::shared_ptr<Resource> LoadResource(const IO::FileLocation& file_location, Args&&... args) {

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);

        // duplicate checking
        if(!resources.emplace(file_location, resource).second)
        {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Tried to insert duplicate resource!");
        }

        //load() success checking
        if(!resource->load())
        {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load resource!");
        }

        return resource;
    }

   private:
    std::unordered_map<IO::FileLocation, std::shared_ptr<Resource>, IO::FileLocationHash> resources;
};
}  // namespace tmt