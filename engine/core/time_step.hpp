#pragma once
#include <cstdint>

namespace tmt {

struct FrameData {
    /* In seconds */
    float delta_time = 0;
    float fixed_delta_time = 0;
    float total_time = 0;

    uint64_t frame_count = 0;
};

}  // namespace tmt