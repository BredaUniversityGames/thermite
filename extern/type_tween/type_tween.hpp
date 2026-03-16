#pragma once

namespace Tweening {

class ITween {
    virtual void update(float dt) = 0;
};

template <typename T>
class TweenStorage {};

template <typename T>
class Tween {
    Tween() = default;
    Tween(T* target)
};

}  // namespace Tweening