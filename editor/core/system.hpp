#pragma once
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
    virtual tmt::json serialize() const;
    virtual void deserialize(const tmt::json value);
};

template <bool Required>
struct TitleMixin {
    virtual std::string get_title() const = 0;
};

template <>
struct TitleMixin<false> {
    virtual std::string get_title() const { return {}; }
};

template <typename Derived = void>
class IEditorSystem : public IEditorSystemBase, public TitleMixin<!std::is_void_v<Derived>> {
   public:

    virtual tmt::json serialize() const {
        if constexpr (std::is_same_v<Derived, void> == false) {
            return Serializer::serialize(static_cast<Derived&>(*this));
        }
        return {};
    }
    virtual void deserialize(const tmt::json value) {
        if constexpr (std::is_same_v<Derived, void> == false) {
            Serializer::deserialize(value, static_cast<Derived&>(*this));
        }
    }
};

}  // namespace tmt
