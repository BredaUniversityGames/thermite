#pragma once
#include <stdint.h>

namespace Tweening {

enum class Ease : uint8_t {
    LINEAR,
    IN_SINE,
    OUT_SINE,
    IN_OUT_SINE,
    IN_QUAD,
    OUT_QUAD,
    IN_OUT_QUAD,
    IN_CUBIC,
    OUT_CUBIC,
    IN_OUT_CUBIC,
    IN_QUART,
    OUT_QUART,
    IN_OUT_QUART,
    IN_QUINT,
    OUT_QUINT,
    IN_OUT_QUINT,
    IN_EXPO,
    OUT_EXPO,
    IN_OUT_EXPO,
    IN_CIRC,
    OUT_CIRC,
    IN_OUT_CIRC,
    IN_BACK,
    OUT_BACK,
    IN_OUT_BACK,
    IN_ELASTIC,
    OUT_ELASTIC,
    IN_OUT_ELASTIC,
    IN_BOUNCE,
    OUT_BOUNCE,
    IN_OUT_BOUNCE
};

template <typename T>
class EaseMixin {
   public:
    T& ease(Ease easing) {
        ease_value = easing;
        return static_cast<T&>(*this);
    }

   private:
    Ease ease_value = Ease::LINEAR;
};

template <typename T>
class Required : public EaseMixin<T> {};

class ITweenBase {
    virtual void interpolate(float t) = 0;
};

template <typename T>
class ITween : public ITweenBase, public Required<T> {};

template <typename T>
class Tween {
    Tween() = default;
    Tween(T* target)
};

}  // namespace Tweening