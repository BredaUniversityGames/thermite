#pragma once
#include <../type_tween/type_tween.hpp>
#include <variant>

#include "engine/core/components/transform.hpp"

template <>
struct Tweening::ITween<tmt::Entity, tmt::Transform> : public Tweening::Detail::TweenBase<ITween<tmt::Entity, tmt::Transform>> {
    enum class Space {
        LOCAL,
        WORLD,
    };

    /* Sub mixins */
    template <typename Derived>
    struct WorldOrLocalMixin {
        Derived& local_space() {
            space = Space::LOCAL;
            return static_cast<Derived&>(*this);
        }
        Derived& world_space() {
            space = Space::WORLD;
            return static_cast<Derived&>(*this);
        }

        Space get_space() const { return space; }

       protected:
        Space space = Space::LOCAL; /* Default to local space */
    };

    template <typename Derived, typename T>
    struct OffsetMixin {
        Derived& offset(const T& offset_val) {
            this->offset_value = offset_val;
            return static_cast<Derived&>(*this);
        }

        bool has_offset() const { return offset_value.has_value(); }

        const T& get_offset() const { return offset_value.value(); }

       protected:
        std::optional<T> offset_value {};
    };

    /* Sub tweens */
    struct Move : public Tweening::Detail::FromToMixin<Move, glm::vec3>,  //
                  public Tweening::Detail::TypedCallbackMixin<Move, tmt::Transform&>,
                  public Tweening::Detail::SubTweenMixin<ITween>,
                  public WorldOrLocalMixin<Move>,
                  public OffsetMixin<Move, glm::vec3> {
        Move() = default;
        Move(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

    struct Rotate : public Tweening::Detail::FromToMixin<Rotate, glm::quat>,  //
                    public Tweening::Detail::TypedCallbackMixin<Rotate, tmt::Transform&>,
                    public Tweening::Detail::SubTweenMixin<ITween>,
                    public WorldOrLocalMixin<Rotate>,
                    public OffsetMixin<Rotate, glm::quat> {
        Rotate() = default;
        Rotate(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

    struct Scale : public Tweening::Detail::FromToMixin<Scale, glm::vec3>,  //
                   public Tweening::Detail::TypedCallbackMixin<Scale, tmt::Transform&>,
                   public Tweening::Detail::SubTweenMixin<ITween>,
                   public WorldOrLocalMixin<Scale>,
                   public OffsetMixin<Scale, glm::vec3> {
        Scale() = default;
        Scale(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

   public:
    ITween(const tmt::Entity entity) : owner(entity) {};
    static ITween create(const tmt::Entity& entity) { return ITween(entity); }

    void interpolate(const Tweening::Detail::TweenFrame& frame) {
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(owner);
        const float t = frame.alpha;

        if (frame.frame_count == 0) {
            /* On first frame, if start or end are not set, initialize them to current value */
            /* If we have an offset, use start + offset as end value */
            std::visit(
                [&](auto& sub) {
                    using SubType = std::decay_t<decltype(sub)>;
                    const bool has_offset = sub.has_offset();
                    const bool is_local = (sub.get_space() == Space::LOCAL);

                    if constexpr (std::is_same_v<SubType, Move>) {
                        if (!sub.has_start()) sub.from(is_local ? transform.get_local_position() : transform.get_world_position());
                        if (has_offset)
                            sub.to(sub.get_start() + sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(is_local ? transform.get_local_position() : transform.get_world_position());
                    } else if constexpr (std::is_same_v<SubType, Rotate>) {
                        if (!sub.has_start()) sub.from(is_local ? transform.get_local_rotation() : transform.get_world_rotation());
                        if (has_offset)
                            sub.to(sub.get_start() * sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(is_local ? transform.get_local_rotation() : transform.get_world_rotation());
                    } else if constexpr (std::is_same_v<SubType, Scale>) {
                        if (!sub.has_start()) sub.from(is_local ? transform.get_local_scale() : transform.get_world_scale());
                        if (has_offset)
                            sub.to(sub.get_start() * sub.get_offset());
                        else if (!sub.has_end())
                            sub.to(is_local ? transform.get_local_scale() : transform.get_world_scale());
                    }
                },
                tween_variant
            );
        }

        std::visit(
            [&](auto& sub) {
                using SubType = std::decay_t<decltype(sub)>;
                const bool is_local = (sub.get_space() == Space::LOCAL);
                if constexpr (std::is_same_v<SubType, Move>) {
                    if (is_local) {
                        transform.set_local_position(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_position(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    }
                } else if constexpr (std::is_same_v<SubType, Rotate>) {
                    if (is_local) {
                        transform.set_local_rotation(glm::slerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_rotation(glm::slerp(sub.get_start(), sub.get_end(), t));
                    }
                } else if constexpr (std::is_same_v<SubType, Scale>) {
                    if (is_local) {
                        transform.set_local_scale(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    } else {
                        transform.set_world_scale(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                    }
                }

                sub.call_update(t, transform);
            },
            tween_variant
        );
    }

    Move& move() {
        tween_variant.emplace<Move>(this);
        return std::get<Move>(tween_variant);
    }

    Rotate& rotate() {
        tween_variant.emplace<Rotate>(this);
        return std::get<Rotate>(tween_variant);
    }

    Scale& scale() {
        tween_variant.emplace<Scale>(this);
        return std::get<Scale>(tween_variant);
    }

   private:
    tmt::Entity owner = entt::null;

    std::variant<Move, Rotate, Scale> tween_variant;
};