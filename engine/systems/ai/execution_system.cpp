#include "execution_system.hpp"
#include "core/logger.hpp"

#include "engine.hpp"
#include "core/ecs.hpp"

#include "tools/fmt_defines.hpp"

namespace tmt {

void Execution::on_start() { Log::info("Action Execution on_start"); }

void Execution::on_update(const FrameData&) {}

void Execution::on_fixed_update(const FrameData&) {}

void Execution::on_end() { Log::info("Action Execution on_end"); }

}  // namespace tmt
