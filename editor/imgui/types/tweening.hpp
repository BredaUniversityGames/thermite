#pragma once
#include "engine/tools/tweening.hpp"
#include <ImReflect.hpp>

template <typename T>
requires std::derived_from<T, Tweening::Detail::EaseMixin<T>>
inline void ease_input(T* value, ImSettings& settings, ImResponse& response) {
    // Fix: value is a pointer, so use -> not .
    ImReflect::Input("Ease", value->get_ease(), settings, response);
}

VISITABLE_TEMPLATE_STRUCT((typename D), Tweening::Detail::EaseMixin, (D), ease_value, reverse_ease_value, has_reverse_ease);

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
        ease_input(&*value, settings, response);
    }
}
