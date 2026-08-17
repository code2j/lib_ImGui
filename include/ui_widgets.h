#pragma once

#include "imgui.h"
#include "eigen3/Eigen/Dense"


namespace ImGui
{
    // ===================================================
    // Collapsing Header(Animated)
    // ====================================================
    bool BeginCollapsingHeader(const char* label, bool default_open = false);
    void EndCollapsingHeader(const char* label);

    // ====================================================
    // Title Button
    // ====================================================
    bool TitleButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags);


    // ====================================================
    // Slider
    // ====================================================
    bool SliderFloatRange(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format = "%.3f");


    // ====================================================
    // Toggle Button
    // ====================================================
    bool ToggleButton(const char* str_id, bool* v);


    // ====================================================
    // Status Step Bar
    // ====================================================
    bool StatusStepBar(const char* str_id, int* current_step, const char** step_labels, int num_steps);


    // ====================================================
    // Joystick
    // ====================================================
    bool Joystic(ImVec2* out);


    // ====================================================
    // Transform Widget
    // ====================================================
    bool TransformControl(Eigen::Matrix4d* matrix);

    // ====================================================
    // Theme Selector
    // ====================================================
    struct ThemePreviewData
    {
        const char* Name;
        ImU32 BgColor;
        ImU32 PanelColor;
        ImU32 PrimaryColor;
        ImU32 SecondaryColor;
        ImU32 TextColor;
    };
    bool ThemeSelector(int* current_theme);

}