#pragma once
#include "engine/tools/tweening.hpp"
#include <ImReflect.hpp>

VISITABLE_TEMPLATE_STRUCT_IN_CONTEXT(ImReflect::Detail::ImContext, (typename D), Tweening::Detail::EaseMixin, (D), ease_value, reverse_ease_value);

template <typename T /*, typename... Args*/>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, Tweening::TypedTween<T /*, Args...*/>& value, ImSettings& settings, ImResponse& response) {
    using TweenTypeHandle = Tweening::TypedTween<T /*, Args...*/>;
    using TweenType = typename TweenTypeHandle::TweenType;

    ImGui::SeparatorText(label);
    ImReflect::Detail::scope_indent();

    if (!value.is_valid()) {
        ImGui::Text("Invalid Tween");
        return;
    }

    constexpr bool HAS_EASE = std::derived_from<TweenType, Tweening::Detail::EaseMixin<TweenType>>;
    if constexpr (HAS_EASE) {
        ImReflect::Input("", static_cast<Tweening::Detail::EaseMixin<TweenType>&>(*value), settings, response);
    }
}
