#include "goap_action_overrides.hpp"

#include <string>
#include <fstream>

//#include "engine/tools/"

namespace tmt {

GoapActionOverrides& GoapActionOverrides::instance() {
    static GoapActionOverrides inst;
    return inst;
}

GoapActionEditorData& GoapActionOverrides::get(const std::string& id) { return data[id]; }

const GoapActionEditorData* GoapActionOverrides::find(const std::string& id) const {
    auto it = data.find(id);
    return (it != data.end()) ? &it->second : nullptr;
}


void GoapActionOverrides::load() {
    std::ifstream f("goap_actions.json");
    if (!f.is_open()) return;

    JsonReflect::json j;
    f >> j;
    //JsonReflect::from_json(j, *this);
    Serializer::deserialize(j, *this);
}

void GoapActionOverrides::save() const {
    //JsonReflect::json j = JsonReflect::to_json(*this);
    tmt::json j = Serializer::serialize(*this);
    std::ofstream f("goap_actions.json");
    f << j.dump(2);
}

}  // namespace tmt
