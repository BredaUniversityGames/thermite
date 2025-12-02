#include "goal_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include "tools/fmt_defines.hpp"

namespace tmt {

void Goal::on_start() { Log::info("Goal on_start"); }

void Goal::on_update(const FrameData&) {}

void Goal::on_fixed_update(const FrameData&) {}

void Goal::on_end() { Log::info("Goal on_end"); }

}  // namespace tmt
