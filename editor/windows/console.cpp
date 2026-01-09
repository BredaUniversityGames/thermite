#include "console.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/input/input.hpp"

using namespace tmt;

// temporary bandaid fix to the default parameter problem
namespace csys {
inline ItemLog& operator<<(ItemLog& log, const glm::vec3& v) {
    log << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return log;
}
}  // namespace csys
void Console::display() { console.DrawContent(); }

void Console::on_editor_start() {
    sink = std::make_shared<ConsoleSink_st>(console.System());
    sink->set_pattern("[%n] %v");
    Log::add_sink(sink);

    register_variables();
    register_commands();
    register_script("example_script", (std::filesystem::path(tmt::IO::get_sub_location_path(tmt::IO::Location::EDITOR)) / "console_scripts" / "example.script").string());
}

void Console::on_editor_update(const FrameData& time) {}

void Console::on_editor_end() { Log::remove_sink(sink); }
void Console::register_variables() {}
