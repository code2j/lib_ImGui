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
    // Widget
    // ====================================================
    bool ButtonX(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags);
    bool CheckboxX(const char* label, bool* v);
    bool RadioButtonX(const char* label, int* v, int v_button);
    bool DragScalarX(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
    bool Drag(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.1f", ImGuiSliderFlags flags = 0);

    bool _combo_(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items = -1);

    bool ComboX(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);

    // 원본 슬라이더
    bool SliderScalarX(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = "%.1f", ImGuiSliderFlags flags = 0);
    bool SliderFloatX(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f", ImGuiSliderFlags flags = 0);

    // 얇은 막대 슬라이더
    bool Slider(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f", ImGuiSliderFlags flags = 0);
    bool SliderX(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f", ImGuiSliderFlags flags = 0);

    // 얇은 막대 슬라이더 범위
    bool SliderRange(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format = "%.1f");
    bool SliderRangeX(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format = "%.1f");


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