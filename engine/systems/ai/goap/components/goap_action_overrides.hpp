#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>

#include "goap_action_editor_data.hpp"
#include "goap_action.hpp"

#include "engine/core/reflection.hpp"


namespace tmt {

class GoapActionOverrides {
   public:
    std::unordered_map<std::string, GoapActionEditorData > data;

    static GoapActionOverrides& instance();
    GoapActionEditorData& get(const std::string& id);
    const GoapActionEditorData* find(const std::string& id) const;

    void load();
    void save() const;

   private:
};

}  // namespace tmt

TMT_OBJECT(tmt::GoapActionOverrides, (data));
