#pragma once
#include <map>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <iostream>

namespace tmt {

template <typename K, typename V, bool ORDERED = true>
class TreeMap {
   private:
    using MapType = std::conditional_t<ORDERED, std::map<K, TreeMap>, std::unordered_map<K, TreeMap>>;

    std::optional<V> value_;
    MapType children_;

   public:
    // ============= CONSTRUCTORS =============
    TreeMap() : value_(std::nullopt), children_() {}

    TreeMap(const V& val) : value_(val), children_() {}

    TreeMap(const MapType& map) : value_(std::nullopt), children_(map) {}

    TreeMap(const K& key, const V& val) : value_(std::nullopt), children_ {{key, TreeMap(val)}} {}

    // ============= CHILD ACCESS =============
    TreeMap& operator[](const K& key) { return children_[key]; }

    TreeMap& get(const K& key) { return children_[key]; }

    TreeMap& at(const K& key) { return children_.at(key); }

    const TreeMap& at(const K& key) const { return children_.at(key); }

    bool contains(const K& key) const { return children_.contains(key); }

    // ============= VALUE MANAGEMENT =============
    TreeMap& operator=(const V& val) {
        value_ = val;
        return *this;
    }

    void set_value(const V& val) { value_ = val; }

    std::optional<V> get_value() const { return value_; }

    V& value() {
        if (!value_.has_value()) {
            throw std::runtime_error("Node does not have a value");
        }
        return *value_;
    }

    const V& value() const {
        if (!value_.has_value()) {
            throw std::runtime_error("Node does not have a value");
        }
        return *value_;
    }

    void clear_value() { value_ = std::nullopt; }

    bool has_value() const { return value_.has_value(); }

    // ============= NODE STATE QUERIES =============
    bool has_children() const { return !children_.empty(); }

    size_t child_count() const { return children_.size(); }

    // Legacy compatibility (can now both be true!)
    bool is_leaf() const { return value_.has_value(); }

    bool is_branch() const { return !children_.empty(); }

    bool has_branches() const { return !children_.empty(); }

    // ============= ITERATION =============
    auto begin() { return children_.begin(); }
    auto end() { return children_.end(); }
    auto begin() const { return children_.begin(); }
    auto end() const { return children_.end(); }

    // ============= UTILITIES =============
    void print(const std::string& prefix = "", const K& key = K {}, bool isRoot = true) const {
        if (!isRoot) {
            std::cout << prefix << "Key: " << key;
            if (value_.has_value()) {
                std::cout << " -> Value: " << *value_;
            }
            std::cout << '\n';
        }

        for (const auto& [childKey, childNode] : children_) {
            childNode.print(prefix + "  ", childKey, false);
        }
    }
};

}  // namespace tmt