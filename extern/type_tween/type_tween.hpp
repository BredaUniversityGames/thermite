#pragma once
#include <stdint.h>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <vector>
#include <numbers>

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

/* Core type */
template <typename T, typename... Args>
class ITween;

namespace Traits {

/* = operator */
template <typename T, typename... Args>
concept HasEqualOperator = requires(T a, T b) {
    { a = b } -> std::same_as<T&>;
};

/* + operator */
template <typename T, typename... Args>
concept HasAddOperator = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

/* - operator */
template <typename T, typename... Args>
concept HasSubOperator = requires(T a, T b) {
    { a - b } -> std::same_as<T>;
};

/* * operator */
template <typename T, typename... Args>
concept HasMulOperator = requires(T a, float b) {
    { a * b } -> std::same_as<T>;
};

/* Lerpable concept, requires all operators needed for linear interpolation */
template <typename T, typename... Args>
concept IsLerpable = HasEqualOperator<T, Args...> && HasAddOperator<T, Args...> && HasSubOperator<T, Args...> && HasMulOperator<T, Args...>;

/* Concept to check if ITween specialization exists for type T and Args... */
template <typename T, typename... Args>
concept HasITween = requires { sizeof(ITween<std::remove_cvref_t<T>, Args...>); };

}  // namespace Traits

namespace Detail {

/* Default context, used by default */
struct Default {};

/* Helpers */
template <typename T>
inline void lerp(T& v, const T& a, const T& b, const float t) {
    /* Linear interpolation */
    v = a + (b - a) * t;
}

template <typename T>
inline T lerp(const T& a, const T& b, const float t) {
    /* Linear interpolation */
    return a + (b - a) * t;
}

/* easing function, defined at bottom of file */
inline float apply_ease(float t, Ease ease);

/* Mixins */
template <typename D>
class EaseMixin {
   public:
    D& ease(const Ease easing) {
        ease_value = easing;
        return static_cast<D&>(*this);
    }

    Ease get_ease() const { return ease_value; }

   protected:
    Ease ease_value = Ease::LINEAR;
};

template <typename D>
class TimeMixin {
   public:
    D& duration(const float seconds) {
        duration_value = seconds;
        return static_cast<D&>(*this);
    }

    float get_duration() const { return duration_value; }

    float& get_elapsed_time() { return elapsed_time; }

   protected:
    float duration_value = 1.0f; /* Default duration is 1 second */

    /* Internal timer */
    float elapsed_time = 0.0f;
};

template <typename D, typename T>
class FromToMixin {
   public:
    D& from(T start) {
        this->start = std::move(start);
        return static_cast<D&>(*this);
    }

    D& to(T end) {
        this->end = std::move(end);
        return static_cast<D&>(*this);
    }

    const T& get_start() const { return start; }

    const T& get_end() const { return end; }

   protected:
    T start {};
    T end {};
};

/* Manager that stores collections of tweens for each context. */
template <typename Context = Detail::Default>
class Manager {
   public:
    static void update(const float delta_time_seconds) {
        for (auto& [_, collection] : collections) {
            collection(delta_time_seconds);
        }
    }

    template <typename CollectionType>
    static void register_collection() {
        const std::type_index type_index(typeid(CollectionType));

        if (collections.find(type_index) != collections.end()) {
            return;  // Collection already registered
        }

        collections[type_index] = [](float delta_time) { CollectionType::update(delta_time); };
    }

   private:
    static inline std::unordered_map<std::type_index, std::function<void(float)>> collections;
};

/* Collection that stores all tween of a specialization */
template <typename Context, typename T, typename... Args>
class TweenCollection {
    using TweenType = ITween<T, Args...>;

   public:
    static TweenType& add(TweenType tween) {
        ensure_registered();
        tweens.push_back(std::move(tween));
        return tweens.back();
    }

    static void update(const float delta_time) {
        // Update each tween

        constexpr bool HAS_EASE = std::derived_from<TweenType, EaseMixin<TweenType>>;
        constexpr bool HAS_TIME = std::derived_from<TweenType, TimeMixin<TweenType>>;

        for (size_t i = 0; i < tweens.size(); ++i) {
            auto& tween = tweens[i];

            float progress = 0.0f;
            if constexpr (HAS_TIME) {
                const float duration = tween.get_duration();
                if (duration > 0.0f) {
                    progress = tween.get_elapsed_time() / duration;
                }
            }

            float t = progress;
            if constexpr (HAS_EASE) {
                t = apply_ease(progress, tween.get_ease());
            }

            /* 0 - 1 */
            tween.interpolate(t);
            tween.get_elapsed_time() += delta_time;
        }
    }

   private:
    static inline std::vector<TweenType> tweens {};

    static void ensure_registered() {
        static bool registered = [] {
            Detail::Manager<Context>::template register_collection<TweenCollection<Context, T, Args...>>();
            return true;
        }();
        (void)registered;
    }
};

}  // namespace Detail

/* Update default context */
inline void update(const float delta_time_seconds) {
    Detail::Manager<>::update(delta_time_seconds);
}

/* Update specific context */
template <typename Context>
inline void update(const float delta_time_seconds) {
    Detail::Manager<Context>::update(delta_time_seconds);
}

/* Entry point, requires ITween type */
template <typename T, typename... Args>
requires Traits::HasITween<T, Args...>
ITween<std::remove_cvref_t<T>, Args...>& tween(T& value) {
    using BareT = std::remove_cvref_t<T>;
    return Detail::TweenCollection<Detail::Default, BareT, Args...>::add(ITween<BareT, Args...>::create(value));
}

/* Error fallback for types that don't have ITween specialization */
template <typename T, typename... Args>
void tween(T& value) {
    static_assert(Traits::HasITween<T, Args...>, "No ITween specialization found for type T and Args...");
}

/* Default Implementation */
template <typename T>
requires Traits::IsLerpable<T>
struct ITween<T> : public Detail::EaseMixin<ITween<T>>,      /**/
                   public Detail::FromToMixin<ITween<T>, T>, /**/
                   public Detail::TimeMixin<ITween<T>> {
    ITween(T* value) : value(value) {};

    static ITween create(T& value) { return ITween(&value); }

    void interpolate(const float t) {
        if (value) {
            Detail::lerp(*value, this->start, this->end, t);
        }
    }

   private:
    T* value = nullptr;
};

}  // namespace Tweening

/* Hide long implementation details */
namespace Tweening::Detail {

inline float apply_ease(float t, Ease ease) {
    using std::cos;
    using std::pow;
    using std::sin;
    using std::sqrt;

    constexpr auto TWEEN_PI = std::numbers::pi_v<float>;

    switch (ease) {
        // Linear
        case Ease::LINEAR:
            return t;

        // Sine
        case Ease::IN_SINE:
            return 1.0f - cos((t * TWEEN_PI) / 2.0f);
        case Ease::OUT_SINE:
            return sin((t * TWEEN_PI) / 2.0f);
        case Ease::IN_OUT_SINE:
            return -(cos(TWEEN_PI * t) - 1.0f) / 2.0f;

        // Quadratic
        case Ease::IN_QUAD:
            return t * t;
        case Ease::OUT_QUAD:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case Ease::IN_OUT_QUAD:
            return (t < 0.5f) ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        // Cubic
        case Ease::IN_CUBIC:
            return t * t * t;
        case Ease::OUT_CUBIC:
            return 1.0f - pow(1.0f - t, 3.0f);
        case Ease::IN_OUT_CUBIC:
            return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

        // Quartic
        case Ease::IN_QUART:
            return pow(t, 4.0f);
        case Ease::OUT_QUART:
            return 1.0f - pow(1.0f - t, 4.0f);
        case Ease::IN_OUT_QUART:
            return (t < 0.5f) ? 8.0f * pow(t, 4.0f) : 1.0f - pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;

        // Quintic
        case Ease::IN_QUINT:
            return pow(t, 5.0f);
        case Ease::OUT_QUINT:
            return 1.0f - pow(1.0f - t, 5.0f);
        case Ease::IN_OUT_QUINT:
            return (t < 0.5f) ? 16.0f * pow(t, 5.0f) : 1.0f - pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;

        // Exponential
        case Ease::IN_EXPO:
            return (t == 0.0f) ? 0.0f : pow(2.0f, 10.0f * t - 10.0f);
        case Ease::OUT_EXPO:
            return (t == 1.0f) ? 1.0f : 1.0f - pow(2.0f, -10.0f * t);
        case Ease::IN_OUT_EXPO:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return (t < 0.5f) ? pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;

        // Circular
        case Ease::IN_CIRC:
            return 1.0f - sqrt(1.0f - t * t);
        case Ease::OUT_CIRC:
            return sqrt(1.0f - pow(t - 1.0f, 2.0f));
        case Ease::IN_OUT_CIRC:
            return (t < 0.5f) ? (1.0f - sqrt(1.0f - pow(2.0f * t, 2.0f))) / 2.0f : (sqrt(1.0f - pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;

        // Back
        case Ease::IN_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }
        case Ease::OUT_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            float tm = t - 1.0f;
            return 1.0f + c3 * tm * tm * tm + c1 * tm * tm;
        }
        case Ease::IN_OUT_BACK: {
            constexpr float c1 = 1.70158f;
            constexpr float c2 = c1 * 1.525f;
            return (t < 0.5f) ? (pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f : (pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2) + 2.0f) / 2.0f;
        }

        // Elastic
        case Ease::IN_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return -pow(2.0f, 10.0f * t - 10.0f) * sin((t * 10.0f - 10.75f) * (2.0f * TWEEN_PI) / 3.0f);
        case Ease::OUT_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return pow(2.0f, -10.0f * t) * sin((t * 10.0f - 0.75f) * (2.0f * TWEEN_PI) / 3.0f) + 1.0f;
        case Ease::IN_OUT_ELASTIC:
            if (t == 0.0f || t == 1.0f) return t;
            return (t < 0.5f) ? -(pow(2.0f, 20.0f * t - 10.0f) * sin((20.0f * t - 11.125f) * (2.0f * TWEEN_PI) / 4.5f)) / 2.0f
                              : (pow(2.0f, -20.0f * t + 10.0f) * sin((20.0f * t - 11.125f) * (2.0f * TWEEN_PI) / 4.5f)) / 2.0f + 1.0f;
        // Bounce
        case Ease::OUT_BOUNCE: {
            if (t < 1.0f / 2.75f) {
                return 7.5625f * t * t;
            } else if (t < 2.0f / 2.75f) {
                t -= 1.5f / 2.75f;
                return 7.5625f * t * t + 0.75f;
            } else if (t < 2.5f / 2.75f) {
                t -= 2.25f / 2.75f;
                return 7.5625f * t * t + 0.9375f;
            } else {
                t -= 2.625f / 2.75f;
                return 7.5625f * t * t + 0.984375f;
            }
        }
        case Ease::IN_BOUNCE:
            return 1.0f - apply_ease(1.0f - t, Ease::OUT_BOUNCE);
        case Ease::IN_OUT_BOUNCE:
            return (t < 0.5f) ? (1.0f - apply_ease(1.0f - 2.0f * t, Ease::OUT_BOUNCE)) / 2.0f : (1.0f + apply_ease(2.0f * t - 1.0f, Ease::OUT_BOUNCE)) / 2.0f;

        default:
            return t;
    }
}

}  // namespace Tweening::Detail