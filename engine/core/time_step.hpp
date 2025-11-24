#pragma once

namespace tmt {

struct TimeStep {
    TimeStep(const float& delta_time_seconds)
        : delta_time(delta_time_seconds) {}

    /* In seconds */
    const float delta_time = 0;

    explicit operator float() const { return delta_time; }

    const float as_seconds() const { return delta_time; }
    const float as_milliseconds() const { return delta_time * 1000.0f; }
};

}  // namespace tmt