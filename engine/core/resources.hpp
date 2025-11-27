#pragma once
#include <memory>
#include "resource.hpp"

namespace tmt {

template <typename T>
concept ResourceType = std::derived_from<T, Resource>;

class ResourceSystem {


   public:
    template <ResourceType T, typename... Args>
    requires std::constructible_from<T, const IO::FileLocation&, Args...>
    std::shared_ptr<Resource> LoadResource(const IO::FileLocation& file_location, Args&&... args) {

        std::shared_ptr<T> resource = std::make_shared<T>(file_location, std::forward<Args>(args)...);

        resource->load();

        resources.emplace(file_location, resource);

        return resource;
    }

   private:
    std::unordered_map<IO::FileLocation, std::shared_ptr<Resource>, IO::FileLocationHash> resources;
};
}  // namespace tmt