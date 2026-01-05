#pragma once
#include <nlohmann/json.hpp>
#include "engine/core/resource.hpp"

namespace tmt {
class Json : public FileResource {
   public:
    Json(IO::FileLocation file_location) : FileResource(std::move(file_location)) {};

    bool load() override;
    void unload() override;

    inline static const std::set<std::string_view> supported_file_extensions {".json"};

    const nlohmann::ordered_json& get_parsed_json() const { return parsed_json; }

   private:
    nlohmann::ordered_json parsed_json;
};
}  // namespace tmt