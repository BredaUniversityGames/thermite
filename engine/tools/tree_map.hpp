#pragma once
#include <map>
#include <unordered_map>
#include <optional>
#include <variant>
#include <stdexcept>

namespace tmt {

template <typename K, typename V, bool ORDERED = true>
class TreeMap {
   private:
    using MapType = std::conditional_t<ORDERED, std::map<K, TreeMap>, std::unordered_map<K, TreeMap>>;
    std::variant<V, MapType> data_;

   public:
    TreeMap() : data_(MapType {}) {}
    TreeMap(const V& val) : data_(val) {}
    TreeMap(const MapType& map) : data_(map) {}
    TreeMap(const K& key, const V& val) : data_(MapType {{key, TreeMap(val)}}) {}

    TreeMap& operator[](const K& key) {
        if (std::holds_alternative<V>(data_)) {
            data_ = MapType {};  // Convert leaf to branch
        }
        return std::get<MapType>(data_)[key];
    }

    const TreeMap& at(const K& key) const {
        if (!std::holds_alternative<MapType>(data_)) {
            throw std::runtime_error("Cannot navigate through a leaf node");
        }
        return std::get<MapType>(data_).at(key);
    }

    TreeMap& operator=(const V& val) {
        data_ = val;
        return *this;
    }

    void set_value(const V& val) { data_ = val; }

    std::optional<V> get_value() const { return std::holds_alternative<V>(data_) ? std::optional<V>(std::get<V>(data_)) : std::nullopt; }

    const V& value() const {
        if (!std::holds_alternative<V>(data_)) {
            throw std::runtime_error("Node is not a leaf");
        }
        return std::get<V>(data_);
    }

    bool contains(const K& key) const {
        auto* map = std::get_if<MapType>(&data_);
        return map && map->contains(key);
    }

    bool is_leaf() const { return std::holds_alternative<V>(data_); }
    bool is_branch() const { return std::holds_alternative<MapType>(data_); }
    bool has_branches() const {
        auto* map = std::get_if<MapType>(&data_);
        return map && !map->empty();
    }

    auto begin() {
        if (!std::holds_alternative<MapType>(data_)) {
            throw std::runtime_error("Cannot iterate over a leaf");
        }
        return std::get<MapType>(data_).begin();
    }
    auto end() {
        if (!std::holds_alternative<MapType>(data_)) {
            throw std::runtime_error("Cannot iterate over a leaf");
        }
        return std::get<MapType>(data_).end();
    }

    void print(const std::string& prefix = "", const K& key = K {}, bool isRoot = true) const {
        if (!isRoot) {
            std::cout << prefix << "Key: " << key;
            if (auto* val = std::get_if<V>(&data_)) {
                std::cout << " -> Value: " << *val;
            }
            std::cout << '\n';
        }

        if (auto* map = std::get_if<MapType>(&data_)) {
            for (const auto& [childKey, childNode] : *map) {
                childNode.print(prefix + "  ", childKey, false);
            }
        }
    }
};

}  // namespace tmt