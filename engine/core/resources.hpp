#pragma once
#include <memory>
#include "resource.hpp"

namespace tmt {
class ResourceSystem {
   public:
    template <typename T, typename... Args>
    std::shared_ptr<Resource> LoadResource(const IO::FileLocation& file_location, Args&&... args) {
        static_assert(std::is_base_of_v<Resource, T>, "Type T is not derived from Resource");

        std::shared_ptr<T> resource = std::make_unique<T>(file_location, std::forward<Args>(args)...);

        static_cast<std::shared_ptr<Resource>>(resource)->load();

        resources.emplace(file_location, resource);

        return resource;
    }

   private:
    std::unordered_map<IO::FileLocation, std::shared_ptr<Resource>, IO::FileLocationHash> resources;
};
}  // namespace tmt