#pragma once

#include "engine/tools/tweening.hpp"
#include "engine/tools/serializer.hpp"

//JSON_REFLECT(Tweening::Detail::EaseMixin, ease_value, reverse_ease, has_reverse_ease);

template <typename T, typename... Args>
inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const Tweening::TypedTween<T, Args...>& tween) {
    using TweenType = Tweening::TypedTween<T, Args...>;

    return {};
}

template <typename T, typename... Args>
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, Tweening::TypedTween<T, Args...>& tween) {
    using TweenType = Tweening::TypedTween<T, Args...>;
}

template <typename A, typename B>
struct MyObject {
    A a;
    B b;
};

VISITABLE_TEMPLATE_STRUCT((typename A, typename B), MyObject, (A, B), a, b);