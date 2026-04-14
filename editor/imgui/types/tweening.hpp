#pragma once
#include <ImReflect.hpp>
#include "engine/tools/tweening.hpp"

template <typename TypeTween>
inline void ease_input(TypeTween& value, ImSettings& settings, ImResponse& response) {
    ImReflect::Input("Ease", value.get_ease(), settings, response);
}

template <typename T/*, typename... Args*/>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, Tweening::TypedTween<T/*, Args...*/>& value, ImSettings& settings, ImResponse& response) {
    using TweenTypeHandle = Tweening::TypedTween<T/*, Args...*/>;
    using TweenType = typename TweenTypeHandle::TweenType;

    if (!value.is_valid()) {
        ImGui::Text("%s: Invalid Tween", label);
        return;
    }

    constexpr bool HAS_EASE = std::derived_from<TweenType, Tweening::Detail::EaseMixin<TweenType>>;
    if constexpr (HAS_EASE) {
        ease_input(value, settings, response);
    }
}