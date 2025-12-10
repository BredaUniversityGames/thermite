#pragma once

/* 
Include this file in a component header and define it like this:



*/

#include <JsonReflect.hpp>
#include <ImReflect_macro.hpp>

#include "engine/tools/serializer.hpp"

namespace tmt {
template <typename T>
struct Component {
    static constexpr const char* name = "Unknown";

    // Optional: helper function
    static constexpr const char* get_name() { return name; }
};
}  // namespace tmt

#define EXPAND(...) __VA_ARGS__

#define TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields)  \
    namespace tmt {                                              \
    template <>                                                  \
    struct Component<Type> {                                     \
        static constexpr const char* name = Name;                \
        static constexpr const char* get_name() { return name; } \
    };                                                           \
    }                                                            \
    IMGUI_REFLECT(Type, EXPAND ImguiFields);                     \
    JSON_REFLECT(Type, EXPAND JsonFields);

#define TMT_COMPONENT(Type, Name, Fields) TMT_COMPONENT_IMPL(Type, Name, Fields, Fields)

#define TMT_COMPONENT_EX(Type, Name, JsonFields, ImguiFields) TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields)