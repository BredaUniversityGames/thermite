#pragma once
#include <any>
#include <map>
#include "engine/tools/serializer.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/io.hpp"

namespace tmt {

class PlayerData {
    struct Config {
        static inline const IO::FileLocation FILE_LOCATION { IO::Location::PROJECT, "PlayerData.json" };
    };

    struct TypeOps {
        const std::type_info& type_info;
        const std::function<tmt::json(const std::any&)> serialize;
        const std::function<void(const tmt::json&, std::any&)> deserialize;
    };

    struct Entry {
        std::any value;
        TypeOps ops;
    };

    std::map<std::string, Entry> entries;
    tmt::json data;

   public:
    template <typename T>
    T& get(const std::string& key, T default_value = {}) {
        const bool contains = entries.contains(key);
        if (contains == false) {
            const bool has_json = data.contains(key);
            if (has_json) {
                const tmt::json& j = data.at(key);
                entries.emplace(
                    key,  //
                    Entry {
                        .value = std::any(T{}),
                        .ops = create_ops<T>(),
                    }
                );
                auto& entry = entries.at(key);
                try {
                    entry.ops.deserialize(j, entry.value);
                } catch (const std::exception& e) {
                    tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to deserialize key '{}': {}", key, e.what());
                    entry.value = default_value;
                }
            } else {
                entries.emplace(
                    key,  //
                    Entry {
                        .value = default_value,
                        .ops = create_ops<T>(),
                    }
                );
            }
        }

        auto& entry = entries.at(key);
        if (entry.ops.type_info != typeid(T)) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Type mismatch for key '{}'. Requested type: '{}', actual type: '{}'", key, typeid(T).name(), entry.ops.type_info.name());
            throw std::runtime_error("Type mismatch for key '" + key + "'. Requested type: '" + typeid(T).name() + "', actual type: '" + entry.ops.type_info.name() + "'.");
        }
        return std::any_cast<T&>(entry.value);
    }

    void serialize() {
        const bool success = IO::write_text_file(Config::FILE_LOCATION, data.dump(4));
        if (!success) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to write PlayerData to file '{}'", Config::FILE_LOCATION);
        }
    }

    void deserialize() {}

   private:
    template <typename T>
    TypeOps create_ops() {
        return TypeOps {
            typeid(T),
            [](const std::any& value) -> tmt::json {
                //
                return tmt::Serializer::serialize(std::any_cast<T>(value));
            },
            [](const tmt::json& j, std::any& value) {
                //
                tmt::Serializer::deserialize(j, std::any_cast<T&>(value));
            },
        };
    }
};

}  // namespace tmt