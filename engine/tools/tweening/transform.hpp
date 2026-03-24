#pragma once
#include <../type_tween/type_tween.hpp>
#include <variant>

#include "engine/core/components/transform.hpp"

template <>
struct Tweening::ITween<tmt::Entity, tmt::Transform> : public Tweening::Detail::TweenBase<ITween<tmt::Entity, tmt::Transform>> {
    /* Sub tweens */
    struct Move : public Tweening::Detail::FromToMixin<Move, glm::vec3>,  //
                  public Tweening::Detail::SubTweenMixin<ITween> {
        Move() = default;
        Move(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

    struct Rotate : public Tweening::Detail::FromToMixin<Rotate, glm::quat>,  //
                    public Tweening::Detail::SubTweenMixin<ITween> {
        Rotate() = default;
        Rotate(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

    struct Scale : public Tweening::Detail::FromToMixin<Scale, glm::vec3>,  //
                   public Tweening::Detail::SubTweenMixin<ITween> {
        Scale() = default;
        Scale(ITween* parent) : Tweening::Detail::SubTweenMixin<ITween>(parent) {}
    };

   public:
    ITween(const tmt::Entity entity) : owner(entity) {};
    static ITween create(tmt::Entity& entity) { return ITween(entity); }

    void interpolate(const float t) {
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(owner);
        std::visit(
            [&](auto& sub) {
                using S = std::decay_t<decltype(sub)>;
                if constexpr (std::is_same_v<S, Move>)
                    transform.set_local_position(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
                else if constexpr (std::is_same_v<S, Rotate>)
                    transform.set_local_rotation(glm::slerp(sub.get_start(), sub.get_end(), t));
                else if constexpr (std::is_same_v<S, Scale>)
                    transform.set_local_scale(Tweening::Detail::lerp(sub.get_start(), sub.get_end(), t));
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