#pragma once
#include <stdint.h>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <vector>
#include <numbers>
#include <optional>

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
    IN_OUT_BOUNCE,
};

enum class LoopMode : uint8_t {
    LOOP,
    PING_PONG,
};

/* Forward declarations */
template <typename T, typename... Args>
class ITween; /* Primary template, should be specialized for each type we want to tween */

namespace Detail {

template <typename D>
class ControllableMixin; /* Mixin to inject control functions into ITween specializations and convert to Tween handle */

}

/* Handle to control a tween after creation, can be used to stop, start, restart, or finish the tween. */
class Tween {
   public:
    Tween() = default;

    ~Tween() {
        if (ctrl_release) ctrl_release();
    }

    /* Non - copyable, one handle owns one tween*/
    Tween(const Tween&) = delete;
    Tween& operator=(const Tween&) = delete;

    Tween(Tween&& o) noexcept :
        ctrl_stop(std::move(o.ctrl_stop)),
        ctrl_start(std::move(o.ctrl_start)),
        ctrl_restart(std::move(o.ctrl_restart)),
        ctrl_finish(std::move(o.ctrl_finish)),
        ctrl_release(std::move(o.ctrl_release)),
        ctrl_is_paused(std::move(o.ctrl_is_paused)) {
        /* clear release so it only fires once */
        o.ctrl_release = nullptr;
    }
    Tween& operator=(Tween&&) = default;

    void stop() {
        if (ctrl_stop) ctrl_stop();
    }
    void start() {
        if (ctrl_start) ctrl_start();
    }
    void restart() {
        if (ctrl_restart) ctrl_restart();
    }
    void finish() {
        if (ctrl_finish) ctrl_finish();
    }

    bool is_paused() const { return ctrl_is_paused ? ctrl_is_paused() : false; }

    bool is_valid() const { return ctrl_stop != nullptr; }

   private:
    template <typename D>
    friend class Detail::ControllableMixin;

    Tween(
        std::function<void()> stop, std::function<void()> start, std::function<void()> restart, std::function<void()> finish, std::function<void()> release, std::function<bool()> is_paused
    ) :
        ctrl_stop(std::move(stop)),
        ctrl_start(std::move(start)),
        ctrl_restart(std::move(restart)),
        ctrl_finish(std::move(finish)),
        ctrl_release(std::move(release)),
        ctrl_is_paused(std::move(is_paused)) {}

    std::function<void()> ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish, ctrl_release;
    std::function<bool()> ctrl_is_paused;
};

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

struct TweenFrame {
    float alpha; /* In Seconds */
    uint64_t frame_count;
};

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
template <typename Derived>
class EaseMixin {
   public:
    Derived& ease(const Ease easing) {
        ease_value = easing;
        return static_cast<Derived&>(*this);
    }

    Derived& reverse_ease(const Ease easing) {
        reverse_ease_value = easing;
        has_reverse_ease = true;
        return static_cast<Derived&>(*this);
    }

   protected:
    Ease ease_value = Ease::LINEAR;
    Ease reverse_ease_value = Ease::LINEAR;
    bool has_reverse_ease = false;
};

template <typename Derived>
class TimeMixin {
   public:
    Derived& duration(const float seconds) {
        duration_value = seconds;
        return static_cast<Derived&>(*this);
    }

    float get_duration() const { return duration_value; }

    float& get_elapsed_time() { return elapsed_time; }

    float get_elapsed_time() const { return elapsed_time; }

   protected:
    float duration_value = 1.0f; /* Default duration is 1 second */

    /* Internal timer */
    float elapsed_time = 0.0f;
};

template <typename Derived, typename T>
class FromToMixin {
   public:
    Derived& from(T start_val) {
        this->start = std::move(start_val);
        return static_cast<Derived&>(*this);
    }

    Derived& to(T end_val) {
        this->end = std::move(end_val);
        return static_cast<Derived&>(*this);
    }

    bool has_start() const { return start.has_value(); }

    bool has_end() const { return end.has_value(); }

    const T& get_start() const { return start.value(); }

    const T& get_end() const { return end.value(); }

   protected:
    std::optional<T> start {};
    std::optional<T> end {};
};

template <typename Derived>
class LoopMixin {
   public:
    /* Loop = 0 -> 1, 0 -> 1, ... */
    Derived& loop(const LoopMode mode = LoopMode::LOOP) {
        loop_mode = mode;
        return static_cast<Derived&>(*this);
    }

    /* PingPong = 0->1, 1->0, ...*/
    Derived& ping_pong() {
        loop_mode = LoopMode::PING_PONG;
        return static_cast<Derived&>(*this);
    }

    LoopMode get_loop_mode() const { return loop_mode; }

   protected:
    LoopMode loop_mode = LoopMode::LOOP;
};

template <typename Derived>
class RepeatMixin {
   public:
    /*
        0 = no repeat, play once
        -1 = infinite repeat
        >0 = repeat count
    */
    Derived& repeat(const int count) {
        repeat_count = count;
        return static_cast<Derived&>(*this);
    }

    int get_repeat_count() const { return repeat_count; }

   protected:
    int repeat_count = 0;
};

template <typename Derived>
class ControllableMixin {
   public:
    operator Tween() {
        keep_alive = true;
        return Tween(ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish, ctrl_release, ctrl_is_paused);
    }

    void inject_control_fns(
        std::function<void()> stop, std::function<void()> start, std::function<void()> restart, std::function<void()> finish, std::function<void()> release, std::function<bool()> is_paused
    ) {
        ctrl_stop = std::move(stop);
        ctrl_start = std::move(start);
        ctrl_restart = std::move(restart);
        ctrl_finish = std::move(finish);
        ctrl_release = std::move(release);
        ctrl_is_paused = std::move(is_paused);
    }

    void do_stop() { paused = true; }

    void do_start() { paused = false; }

    void do_restart() {
        /* Reach up to into time mixin */
        static_assert(std::derived_from<Derived, TimeMixin<Derived>>, "Restart requires TimeMixin");
        static_cast<Derived*>(this)->get_elapsed_time() = 0.0f;
        paused = false;
    }

    void do_finish() {
        /* Reach up to into time mixin */
        static_assert(std::derived_from<Derived, TimeMixin<Derived>>, "Finish requires TimeMixin");
        static_assert(std::derived_from<Derived, RepeatMixin<Derived>>, "Finish requires RepeatMixin");
        auto* d = static_cast<Derived*>(this);
        d->get_elapsed_time() = d->get_duration() * (d->get_repeat_count() + 1);
    }

    void do_release() { keep_alive = false; }

    bool is_paused() const { return paused; }
    bool is_keep_alive() const { return keep_alive; }

   private:
    bool paused = false;
    bool keep_alive = false;

    std::function<void()> ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish, ctrl_release;
    std::function<bool()> ctrl_is_paused;
};

/* Mixins for sub-tweens to convert to Tween handle */
template <typename Parent>
class SubTweenMixin {
   public:
    SubTweenMixin() = default;
    SubTweenMixin(Parent* parent) : parent(parent) {}

    operator Tween() { return parent->operator Tween(); }

   protected:
    Parent* parent = nullptr;
};

template <typename Derived>
class TweenBase : public EaseMixin<Derived>, /*<br>*/
                  public TimeMixin<Derived>,
                  public LoopMixin<Derived>,
                  public RepeatMixin<Derived>,
                  public ControllableMixin<Derived> {
   public:
    void tick(const float delta_time) {
        if (this->is_paused()) return;
        if (this->is_done()) return;

        auto& elapsed = this->get_elapsed_time();
        const float duration = this->get_duration();

        elapsed += delta_time;

        if (this->get_repeat_count() >= 0) {
            const float total_cycle = (this->get_loop_mode() == LoopMode::PING_PONG) ? duration * 2.0f : duration;
            const float max_elapsed = total_cycle * (this->get_repeat_count() + 1);
            elapsed = std::min(elapsed, max_elapsed);
        }

        float progress = (duration > 0.0f) ? elapsed / duration : 1.0f;

        bool is_reversing = false;
        const float computed = compute_progress(progress, is_reversing);

        float t;
        if (is_reversing && this->has_reverse_ease) {
            t = apply_ease(computed, this->reverse_ease_value);        // fully custom return ease
        } else if (is_reversing) {
            t = 1.0f - apply_ease(1.0f - computed, this->ease_value);  // mirrored default
        } else {
            t = apply_ease(computed, this->ease_value);
        }
        TweenFrame frame { t, frame_count };
        static_cast<Derived*>(this)->interpolate(frame);

        frame_count++;
    }

    bool is_done() const {
        if (this->get_repeat_count() < 0) return false;  // infinite

        const float cycle = (this->get_loop_mode() == LoopMode::PING_PONG) ? this->get_duration() * 2.0f : this->get_duration();

        return this->get_elapsed_time() >= cycle * (this->get_repeat_count() + 1);
    }

   private:
    uint64_t frame_count = 0;

    float compute_progress(float progress, bool& is_reversing) const {
        is_reversing = false;
        switch (this->get_loop_mode()) {
            case LoopMode::LOOP:
                return std::fmod(progress, 1.0f);
            case LoopMode::PING_PONG: {
                const float mod = std::fmod(progress, 2.0f);
                if (mod > 1.0f) {
                    is_reversing = true;
                    return 2.0f - mod;  // goes back from 1->0
                }
                return mod;
            }
            default:
                return std::min(progress, 1.0f);
        }
    }
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
        auto& stored = tweens.back();
        stored.inject_control_fns(
            [&stored] { stored.do_stop(); },     //
            [&stored] { stored.do_start(); },    //
            [&stored] { stored.do_restart(); },  //
            [&stored] { stored.do_finish(); },   //
            [&stored] { stored.do_release(); },  //
            [&stored] { return stored.is_paused(); }
        );
        return stored;
    }

    static void update(const float delta_time) {
        for (auto& tween : tweens) {
            tween.tick(delta_time);
        }

        std::erase_if(tweens, [](const TweenType& tween) { return tween.is_done() && !tween.is_keep_alive(); });
    }

   private:
    /* Using deque to avoid pointer invalidation when storing tweens */
    static inline std::deque<TweenType> tweens {};

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

/* Entry point */
template <typename... Args, typename T>
requires Traits::HasITween<T, Args...>
ITween<std::remove_cvref_t<T>, Args...>& tween(T& value) {
    using BareT = std::remove_cvref_t<T>;
    return Detail::TweenCollection<Detail::Default, BareT, Args...>::add(ITween<BareT, Args...>::create(value));
}

/* Error fallback for types that don't have ITween specialization */
template <typename... Args, typename T>
void tween(T& value) {
    static_assert(Traits::HasITween<T, Args...>, "No ITween specialization found for type T and Args...");
}

/* Default Implementation */
template <typename T>
requires Traits::IsLerpable<T>
struct ITween<T> : public Detail::TweenBase<ITween<T>>, /**/
                   public Detail::FromToMixin<ITween<T>, T> {
    ITween(T* value) : value(value) {};
    static ITween create(T& value) { return ITween(&value); }

    void interpolate(const Detail::TweenFrame& frame) {
        if (frame.frame_count == 0) {
            /* On first frame, if start or end are not set, initialize them to current value */
            if (!this->has_start()) this->from(*value);
            if (!this->has_end()) this->to(*value);
        }
        if (value) {
            Detail::lerp(*value, this->start, this->end, frame.alpha);
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