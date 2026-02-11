#pragma once

#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

namespace tmt {

template <typename Type>
class TypeDiff : public IUndoRedo {
   public:
    // Before change is automatically taken from the current value of the variable.
    TypeDiff(Type* variable, const Type& after) : variable { variable }, before_change { *variable }, after_change { after } {}

    void undo() override { *variable = before_change; }
    void redo() override { *variable = after_change; }

    void inspect() override {}

   private:
    Type* variable;

    Type before_change;
    Type after_change;
};

}  // namespace tmt