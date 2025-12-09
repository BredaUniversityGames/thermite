#pragma once
#include <tuple>

namespace tmt {
template <typename... Components>
class ComponentRegistry {
   public:
    using components = std::tuple<Components...>;

    static constexpr size_t COUNT = sizeof...(Components);

    template <size_t Index>
    using get = std::tuple_element_t<Index, components>;

    template <typename Func>
    static void for_each(Func&& func) {
        std::apply(
            [&](auto&&... components) {
                /* for each component */
                (func(std::decay_t<decltype(components)> {}), ...);
            },
            components {}
        );
    }
};
}  // namespace tmt