#pragma once
#include <type_traits>

#include "engine/core/frame_data.hpp"
#include "engine/tools/json.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/all.hpp"

namespace tmt {

/* For backwards compatibility, default to void */
class IEditorSystemBase {
   public:
    virtual ~IEditorSystemBase() = default;

    /* [Optional] */
    virtual void on_editor_start() {};
    virtual void on_editor_update(const tmt::FrameData& time) { (void)time; };
    virtual void on_editor_fixed_update(const tmt::FrameData& time) { (void)time; };
    virtual void on_editor_end() {};

    /* Optional, defaults to reflection */
    virtual tmt::json serialize() const { return {}; }
    virtual void deserialize(const tmt::json value) { (void)value; }
};

template <bool Required>
struct TitleMixin {
    virtual std::string get_title() const = 0;
};

template <>
struct TitleMixin<false> {
    virtual std::string get_title() const { return {}; }
};

struct temp {};

template <typename Derived = temp>
class IEditorSystem : public IEditorSystemBase, public TitleMixin<!std::is_same_v<Derived, temp>> {
   public:
    virtual tmt::json serialize() const override {
        if constexpr (std::is_same_v<Derived, temp> == false) {
            return Serializer::serialize(static_cast<const Derived&>(*this));
        }
        return {};
    }
    virtual void deserialize(const tmt::json value) override {
        if constexpr (std::is_same_v<Derived, temp> == false) {
            Serializer::deserialize(value, static_cast<Derived&>(*this));
        }
    }
};

}  // namespace tmt
