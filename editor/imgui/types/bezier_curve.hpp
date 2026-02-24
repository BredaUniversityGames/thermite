#pragma once

#include <ImReflect.hpp>

#include "extern/imgui-bezier-editor/bezier_editor.hpp"

#include "engine/tools/types/bezier_curve.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::BezierCurve& curve, ImSettings&, ImResponse& response) {
    static float v[5] = { 0.390f, 0.575f, 0.565f, 1.000f };
    ImGui::Bezier( "easeOutSine", v );       // draw
    float y = ImGui::BezierValue( 0.5f, v ); // x delta in [0..1] range
}