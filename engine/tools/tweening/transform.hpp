#pragma once
#include <../type_tween/type_tween.hpp>
#include <variant>

#include "engine/core/components/transform.hpp"

template <>
struct Tweening::ITween<tmt::Entity> {
    using TypeTween = Tweening::ITween<tmt::Entity>;

    struct Move : public Tweening::Detail::EaseMixin<Move>,                  /**/
                  public Tweening::Detail::FromToMixin<Move, glm::vec3>,     /**/
                  public Tweening::Detail::TimeMixin<Move> {};

    struct Rotate : public Tweening::Detail::EaseMixin<Rotate>,              /**/
                    public Tweening::Detail::FromToMixin<Rotate, glm::quat>, /**/
                    public Tweening::Detail::TimeMixin<Rotate> {};

    struct Scale : public Tweening::Detail::EaseMixin<Scale>,                /**/
                   public Tweening::Detail::FromToMixin<Scale, glm::vec3>,   /**/
                   public Tweening::Detail::TimeMixin<Scale> {};

    using MoveTween = TypeTween::Move;
    using RotateTween = TypeTween::Rotate;
    using ScaleTween = TypeTween::Scale;

   public:
    ITween(const tmt::Entity entity) : owner(entity) {};

    static ITween create(tmt::Entity& entity) { return ITween(entity); }

    void interpolate(const float t) {
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(owner);
        if (std::holds_alternative<Move>(tween_variant)) {
            const Move& move_tween = std::get<Move>(tween_variant);
            const auto new_pos = Tweening::Detail::lerp(move_tween.get_start(), move_tween.get_end(), t);
            transform.set_local_position(new_pos);
        } else if (std::holds_alternative<Rotate>(tween_variant)) {
            const Rotate& rotate_tween = std::get<Rotate>(tween_variant);
            const auto new_rot = glm::slerp(rotate_tween.get_start(), rotate_tween.get_end(), t);
            transform.set_local_rotation(new_rot);
        } else if (std::holds_alternative<Scale>(tween_variant)) {
            const Scale& scale_tween = std::get<Scale>(tween_variant);
            const auto new_scale = Tweening::Detail::lerp(scale_tween.get_start(), scale_tween.get_end(), t);
            transform.set_local_scale(new_scale);
        }
    }

    MoveTween& move() {
        tween_variant.emplace<Move>();
        return std::get<Move>(tween_variant);
    }

    RotateTween& rotate() {
        tween_variant.emplace<Rotate>();
        return std::get<Rotate>(tween_variant);
    }

    ScaleTween& scale() {
        tween_variant.emplace<Scale>();
        return std::get<Scale>(tween_variant);
    }

   private:
    const tmt::Entity owner = entt::null;

    std::variant<Move, Rotate, Scale> tween_variant;
};