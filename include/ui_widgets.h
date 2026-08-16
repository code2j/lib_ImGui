#pragma once

#include "imgui.h"

namespace ImGui
{
    // ===================================================
    // CollapsingHeader(Animated)
    // ====================================================
    bool BeginCollapsingHeader(const char* label, bool default_open = false);
    void EndCollapsingHeader(const char* label);


    // ====================================================
    // Slider
    // ====================================================
    bool SliderFloatRange(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format = "%.3f");



}