#include "plan_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include "tools/fmt_defines.hpp"

namespace tmt {

void Plan::on_start() { Log::info("Plan on_start"); }

void Plan::on_update(const FrameData&) {}

void Plan::on_fixed_update(const FrameData&) {}

void Plan::on_end() { Log::info("Plan on_end"); }

}  // namespace tmt
