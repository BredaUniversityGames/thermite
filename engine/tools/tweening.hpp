#pragma once
#include "engine/tools/serializer.hpp"
#include "engine/tools/component_registry.hpp"

#define BEFRIEND_MIXINS BEFRIEND_VISITABLE();
#include <../type_tween/type_tween.hpp>

namespace tmt {

// clang-format off
template <typename Derived, typename T>
using TweenMixinsSerialize = ComponentRegistry<
    /*Order in which mixins are displayed */
    Tweening::Detail::FromToMixin<Derived, T>,
    Tweening::Detail::EaseMixin<Derived>,
    Tweening::Detail::TimeMixin<Derived>,
    Tweening::Detail::LoopMixin<Derived>,
    Tweening::Detail::RepeatMixin<Derived>
    >;
// clang-format on

// clang-format off
template <typename Derived, typename T>
using TweenMixinsInspect = ComponentRegistry<
    /*Order in which mixins are displayed */
    Tweening::Detail::FromToMixin<Derived, T>,
    Tweening::Detail::EaseMixin<Derived>,
    Tweening::Detail::TimeMixin<Derived>,
    Tweening::Detail::LoopMixin<Derived>,
    Tweening::Detail::RepeatMixin<Derived>,
    Tweening::Detail::ControllableMixin<Derived>
    >;
// clang-format on

}  // namespace tmt

//VISITABLE_TEMPLATE_STRUCT_IN_CONTEXT(ImReflect::Detail::ImContext, (typename D), Tweening::Detail::EaseMixin, (D), ease_value, reverse_ease_value);

TMT_OBJECT_TEMPLATE((typename Derived, typename T), Tweening::Detail::FromToMixin, (Derived, T), start_value, end_value);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::EaseMixin, (Derived), ease_value, reverse_ease_value);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::TimeMixin, (Derived), duration_value, start_delay_value, reverse_delay_value, repeat_delay_value, end_delay_value);
