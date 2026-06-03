#pragma once
#include <any>
#include <map>
#include <typeindex>
#include "engine/tools/serializer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
    #include <ImReflect.hpp>
    #include "editor/imgui/types/all.hpp"
    #include "editor/imgui/components/all.hpp"
#endif

namespace tmt {

class PlayerData {
    struct Config {
        static inline const IO::FileLocation FILE_LOCATION { IO::Location::USERDATA, "PlayerData.json" };
    };

    struct TypeOps {
        std::type_index type_index = typeid(void);
        std::function<tmt::json(const std::any&)> serialize = nullptr;
        std::function<void(const tmt::json&, std::any&)> deserialize = nullptr;
        std::function<void(const char* label, std::any&)> inspect = nullptr;
    };

    struct Entry {
        std::any value = {};
        TypeOps ops = {};
    };

    std::map<std::string, Entry> entries;
    ResourceRef<Json> data;

   public:
    /* Important! Type needs to be reflected */
    template <typename T>
    T& get(const std::string& key, T default_value = {}) {
        if (!entries.contains(key)) {
            emplace_entry<T>(key, std::move(default_value));
        }

        auto& entry = entries.at(key);
        if (entry.ops.type_index != std::type_index(typeid(T))) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Type mismatch for key '{}'. Requested: '{}', actual: '{}'. Overriding...", key, typeid(T).name(), entry.ops.type_index.name());
            entries.erase(key);
            return std::any_cast<T&>(emplace_entry<T>(key, std::move(default_value)).value);
        }

        try {
            return std::any_cast<T&>(entry.value);
        } catch (const std::bad_any_cast& e) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "any_cast failed for key '{}': {}. Overriding with default.", key, e.what());
            entries.erase(key);
            return std::any_cast<T&>(emplace_entry<T>(key, std::move(default_value)).value);
        }
    }

    template <typename T>
    std::optional<T> try_get(const std::string& key) {
        if (!entries.contains(key)) {
            if (!data || !data->get_parsed_json().contains(key)) return std::nullopt;
            emplace_entry<T>(key, T {});
        }

        const auto& entry = entries.at(key);
        if (entry.ops.type_index != std::type_index(typeid(T))) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Type mismatch for key '{}'. Requested: '{}', actual: '{}'. Returning nullopt...", key, typeid(T).name(), entry.ops.type_index.name());
            return std::nullopt;
        }

        try {
            return std::any_cast<T>(entry.value);
        } catch (const std::exception& e) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "any_cast failed for key '{}': {}. Returning nullopt...", key, e.what());
            return std::nullopt;
        }
    }

    template <typename T>
    bool has(const std::string& key) const {
        const bool in_entries = entries.contains(key);
        const bool in_json = data && data->get_parsed_json().contains(key);
        if (!in_entries && !in_json) {
            return false;
        }

        const auto& entry = entries.at(key);
        return entry.ops.type_index == std::type_index(typeid(T));
    }

    void serialize() {
        /* Update JSON data from entries */
        for (const auto& [key, entry] : entries) {
            try {
                data->get_parsed_json()[key] = entry.ops.serialize(entry.value);
            } catch (const std::exception& e) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to serialize key '{}': {}", key, e.what());
            }
        }

        /* Write to disk */
        const bool success = IO::write_text_file(Config::FILE_LOCATION, data->get_parsed_json().dump(4));
        if (!success) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to write PlayerData to file '{}'", Config::FILE_LOCATION);
        }
    }

    void init() {
        data = engine.resources.load_resource<Json>(Config::FILE_LOCATION);
        if (!data) {
            /* Create file and load */
            IO::write_text_file(Config::FILE_LOCATION, "{}");
            data = engine.resources.load_resource<Json>(Config::FILE_LOCATION);
        }
    }

    void clear() {
        entries.clear();
        if (data) {
            data->get_parsed_json().clear();
            IO::write_text_file(Config::FILE_LOCATION, "{}");
        }
    }

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
    void inspect() {
        for (auto& [key, entry] : entries) {
            if (entry.ops.inspect == nullptr) {
                ImGui::Text("Key '%s' with type '%s' has not been called yet or is engine struct.", key.c_str(), entry.ops.type_index.name());
                continue;
            }
            if (entry.value.has_value() == false) {
                ImGui::Text("Key '%s' with type '%s' has no value.", key.c_str(), entry.ops.type_index.name());
                continue;
            }
            try {
                entry.ops.inspect(key.c_str(), entry.value);
            } catch (const std::exception& e) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to inspect key '{}': {}", key, e.what());
            }
        }
    }
#endif

   private:
    template <typename T>
    TypeOps create_ops() {
        return TypeOps {
            std::type_index(typeid(T)),
            [](const std::any& value) -> tmt::json {
                //
                return tmt::Serializer::serialize(std::any_cast<T>(value));
            },
            [](const tmt::json& j, std::any& value) {
                //
                tmt::Serializer::deserialize(j, std::any_cast<T&>(value));
            },
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
            [](const char* label, std::any& value) {
                //
                ImReflect::Input(label, std::any_cast<T&>(value));
            },
#endif
        };
    }

    void load_from_json(const std::string& key) {
        if (!data || !data->get_parsed_json().contains(key)) return;

        auto& entry = entries.at(key);
        try {
            entry.ops.deserialize(data->get_parsed_json().at(key), entry.value);
        } catch (const std::exception& e) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to deserialize key '{}': {}. Using default value.", key, e.what());
        }
    }

    template <typename T>
    Entry& emplace_entry(const std::string& key, T default_value) {
        entries.emplace(
            key, Entry {
                     .value = std::move(default_value),
                     .ops = create_ops<T>(),
                 }
        );
        load_from_json(key);
        return entries.at(key);
    }
};

}  // namespace tmt