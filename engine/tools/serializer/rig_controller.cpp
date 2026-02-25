#include "rig_controller.hpp"

#include "engine/tools/serializer/containers.hpp"
#include "engine/tools/serializer.hpp"

#ifdef THERMITE_EDITOR
#include <extern/imgui-node-editor/imgui_node_editor.h>
#endif  // THERMITE_EDITOR

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::RigController& controller) {
    JsonReflect::json j;
    j["parameters"] = tmt::Serializer::serialize(controller.parameters);
    j["states"] = tmt::Serializer::serialize(controller.states);
    j["transitions"] = tmt::Serializer::serialize(controller.transitions);

#ifdef KUDZU_INSPECTORs
    const std::string editor_context = ax::NodeEditor::Config::SaveSettings(value.editor_context);
    j["editor_context"] = kudzu::JsonSerializer::serialize(editor_context);
#endif  // THERMITE_EDITOR
    return j;
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigController& controller) {
    if (j.contains("parameters")) tmt::Serializer::deserialize(j["parameters"], controller.parameters);
    if (j.contains("states")) tmt::Serializer::deserialize(j["states"], controller.states);
    if (j.contains("transitions")) tmt::Serializer::deserialize(j["transitions"], controller.transitions);

#ifdef KUDZU_INSPECTOR
    if (j.contains("editor_context")) {
        std::string editor_context;
        tmt::Serializer::deserialize(j["editor_context"], editor_context);

        // I have no idea why, but you have to set the current context even though the function takes in a specific context
        // anyway (otherwise it crashes when rendering)
        ax::NodeEditor::EditorContextSet(value.editor_context);
        ax::NodeEditor::LoadEditorStateFromIniString(value.editor_context, editor_context.data(), editor_context.size());
    }
#endif  // KUDZU_INSPECTOR
}
