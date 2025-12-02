#pragma once
#include <memory>
#include "resource.hpp"
#include "logger.hpp"

namespace tmt {

class Resources {
   public:
    /* file resources loading */
    template <ResourceType T, typename... Args>
        requires std::constructible_from<T, const IO::FileLocation&, Args...>
    std::shared_ptr<T> load_resource(const IO::FileLocation& file_location, Args&&... args) {
        // duplicate checking
        if (resources.contains(file_location)) {
            // already loaded
            return std::dynamic_pointer_cast<T>(resources[file_location]);
        }

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);

        // load() success checking
        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load resource!");
            resource->unload();
            return nullptr;
        }

        resources.emplace(file_location, resource);

        return std::dynamic_pointer_cast<T>(resource);
    }

    /* Runtime resources loading that takes in a file resource */
    template <typename T>
        requires std::derived_from<T, RuntimeResource<typename T::ResourceType>>
    std::shared_ptr<T> load_resource(const std::shared_ptr<typename T::ResourceType>& file_resource) {
        std::shared_ptr<T> resource = std::make_shared<T>(file_resource);
        // load() success checking
        if (!resource->load()) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to load runtime resource!");
            resource->unload();
            return nullptr;
        }
        runtime_resources.push_back(resource);
        return resource;
    }

    /* Runtime resources loading that takes in a file resource args */
    template <typename T, typename... Args>
        requires std::derived_from<T, RuntimeResource<typename T::ResourceType>> && std::constructible_from<typename T::ResourceType, const IO::FileLocation&, Args...>
    std::shared_ptr<T> load_resource(const IO::FileLocation& file_location, Args&&... args) {
        auto file_resource = load_resource<typename T::ResourceType>(file_location, std::forward<Args>(args)...);
        if (file_resource == nullptr) {
            return nullptr;
        }
        return load_resource<T>(file_resource);
    }

   private:
    std::unordered_map<IO::FileLocation, std::shared_ptr<FileResource>, IO::FileLocationHash> resources;
    std::vector<std::shared_ptr<Resource>> runtime_resources;
};
}  // namespace tmt