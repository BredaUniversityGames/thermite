#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

struct BezierCurve {
    std::vector<float> values { 0.390f, 0.575f, 0.565f, 1.000f, 0.0f };
};

}  // namespace tmt

TMT_OBJECT(tmt::BezierCurve, (values));