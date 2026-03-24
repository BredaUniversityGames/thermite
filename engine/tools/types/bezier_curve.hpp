#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

struct BezierCurve {
    // Linear by default
    std::vector<float> values { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    static float eval(float x, float* p) {
        float t = x;  // initial guess
        for (int i = 0; i < 5; i++) {
            const float dx = bezier_dx(t, p);
            if (abs(dx) < 1e-6) break;
            t -= (bezier_x(t, p) - x) / dx;
            t = glm::clamp(t, 0.f, 1.f);
        }
        return glm::clamp(bezier_y(t, p), 0.f, 1.f);
    }

    inline glm::vec4 get_vec4() const { return glm::vec4(values[0], values[1], values[2], values[3]); }

   private:
    // Bezier Helpers
    static float bezier_x(float t, const float* p) {
        const float u = 1.0f - t;
        return 3.0f * u * u * t * p[0] + 3.0f * u * t * t * p[2] + t * t * t;
    }
    static float bezier_y(float t, const float* p) {
        const float u = 1.0f - t;
        return 3.0f * u * u * t * p[1] + 3.0f * u * t * t * p[3] + t * t * t;
    }
    static float bezier_dx(float t, const float* p) {
        const float u = 1.0f - t;
        return 3.0f * u * u * p[0] + 6.0f * u * t * (p[2] - p[0]) + 3.0f * t * t * (1.0f - p[2]);
    }
};

}  // namespace tmt

TMT_OBJECT(tmt::BezierCurve, (values));
