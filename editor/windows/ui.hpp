#pragma once
#include "editor/core/window.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

class UIEditor : public IEditorSystem<UIEditor> {
   public:
    UIEditor() = default;
    ~UIEditor() = default;

    void on_editor_update(const FrameData& time) override;

    virtual std::string get_title() const override { return "UI Editor"; }
};

}  // namespace tmt