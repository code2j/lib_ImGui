#include "ui.hpp"


namespace ImGui
{
    void style()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding     = ImVec2(8.00, 8.00); // Window 내측 여백 (Padding)
        style.FramePadding      = ImVec2(5.00, 6.00); // Frame 내측 여백 (Padding)
        style.CellPadding       = ImVec2(6.00, 6.00); // Table Cell 내측 여백 (Padding)
        style.ItemSpacing       = ImVec2(6.00, 6.00); // Item 간의 간격 (Spacing)
        style.ItemInnerSpacing  = ImVec2(6.00, 6.00); // Item 내부 요소 간의 간격 (Inner Spacing)
        style.TouchExtraPadding = ImVec2(0.00, 0.00); // 터치 조작을 위한 추가 여백 (Padding)
        style.IndentSpacing     = 25;                 // 들여쓰기 (Indent) 간격
        style.ScrollbarSize     = 1;                 // Scrollbar 두께/크기
        style.GrabMinSize       = 10;                 // 슬라이더 등 Grab(손잡이)의 최소 크기
        style.WindowBorderSize  = 0;                  // Window 테두리 (Border) 두께
        style.ChildBorderSize   = 1;                  // Child 창 테두리 (Border) 두께
        style.PopupBorderSize   = 1;                  // Popup 테두리 (Border) 두께
        style.FrameBorderSize   = 1;                  // Frame 테두리 (Border) 두께
        style.TabBorderSize     = 0;                  // Tab 테두리 (Border) 두께
        style.WindowRounding    = 3;                  // Window 모서리 둥글기 (Rounding)
        style.ChildRounding     = 12;                  // Child 창 모서리 둥글기 (Rounding)
        style.FrameRounding     = 7;                  // Frame 모서리 둥글기 (Rounding)
        style.PopupRounding     = 4;                  // Popup 모서리 둥글기 (Rounding)
        style.ScrollbarRounding = 9;                  // Scrollbar 모서리 둥글기 (Rounding)
        style.GrabRounding      = 3;                  // Grab(손잡이) 모서리 둥글기 (Rounding)
        style.LogSliderDeadzone = 4;                  // Logarithmic Slider의 데드존(Deadzone) 크기
        style.TabRounding       = 6;                  // Tab 모서리 둥글기 (Rounding)
        style.TabBarBorderSize  = 0;
        // style.WindowMenuButtonPosition = ImGuiDir_None; // 탭 최소화 버튼 제거


        // ---------------------------------------------------------------
        // 안티엘리어싱 설정
        // ---------------------------------------------------------------
        style.AntiAliasedLines          = true;
        style.AntiAliasedFill           = true;
        style.AntiAliasedLinesUseTex    = true;
    }

    void theme_white()
    {
        if (ImGui::theme_id != 0) return;

        auto& colors = ImGui::GetStyle().Colors;

        ImVec4 color_bg                 = ImColor(251, 251, 251); // v
        ImVec4 color_surf               = ImColor(235, 235, 237);
        ImVec4 color_surf_variant       = ImColor(246, 246, 246);

        ImVec4 color_primary            = ImColor(93, 105, 240);
        ImVec4 color_primary_hover      = ImColor(73, 85, 185);
        ImVec4 color_primary_active     = ImColor(63, 74, 162);


        ImVec4 color_red                = ImColor(181, 65, 60);
        ImVec4 color_green              = ImColor(87, 242, 135);

        ImVec4 color_text               = ImColor(40, 40, 45);      // v
        ImVec4 color_text_disabled      = ImColor(128, 133, 138);
        ImVec4 color_transparent        = ImColor(0, 0, 0, 0);


        ImVec4 color_tab_hovered        = ImColor(200, 200, 200);
        ImVec4 color_tab_focused        = ImColor(235, 235, 237); // v
        ImVec4 color_tab_active         = ImColor(235, 235, 237);

        ImVec4 color_border             = ImColor(213, 213, 217); // v

        ImVec4 color_scrollbar          = ImColor(92, 93, 103);
        ImVec4 color_scrollbar_hover    = ImColor(71, 77, 82);
        ImVec4 color_scrollbar_active   = ImColor(82, 87, 92);


        // ---------------------------------------------------------------
        // ImGui 색상 적용
        // ---------------------------------------------------------------
        // [Text]
        colors[ImGuiCol_Text]                  = color_text;
        colors[ImGuiCol_TextDisabled]          = color_text_disabled;
        colors[ImGuiCol_TextSelectedBg]        = color_primary;
        colors[ImGuiCol_DragDropTarget]        = color_primary_active;
        // [Background]
        colors[ImGuiCol_WindowBg]              = color_bg;
        colors[ImGuiCol_ChildBg]               = color_surf;
        colors[ImGuiCol_PopupBg]               = color_surf;
        colors[ImGuiCol_MenuBarBg]             = color_surf;
        // [Border]
        colors[ImGuiCol_Border]                = color_border;
        colors[ImGuiCol_BorderShadow]          = color_transparent;
        // [Frame]
        colors[ImGuiCol_FrameBg]               = color_surf_variant;
        colors[ImGuiCol_FrameBgHovered]        = color_transparent;
        colors[ImGuiCol_FrameBgActive]         = color_transparent;
        // [Title]
        colors[ImGuiCol_TitleBg]               = color_bg;
        colors[ImGuiCol_TitleBgActive]         = color_bg;
        colors[ImGuiCol_TitleBgCollapsed]      = color_bg;
        // [Scrollbar]
        colors[ImGuiCol_ScrollbarBg]           = color_transparent;
        colors[ImGuiCol_ScrollbarGrab]         = color_scrollbar;
        colors[ImGuiCol_ScrollbarGrabHovered]  = color_scrollbar_hover;
        colors[ImGuiCol_ScrollbarGrabActive]   = color_scrollbar_active;
        // [Checkbox]
        colors[ImGuiCol_CheckMark]             = color_primary;
        // [Slider]
        colors[ImGuiCol_SliderGrab]            = color_primary;
        colors[ImGuiCol_SliderGrabActive]      = color_primary_hover;
        // [Button]
        colors[ImGuiCol_Button]                = color_primary;
        colors[ImGuiCol_ButtonHovered]         = color_primary_hover;
        colors[ImGuiCol_ButtonActive]          = color_primary_active;
        // [Header]
        colors[ImGuiCol_Header]                = color_tab_focused;
        colors[ImGuiCol_HeaderHovered]         = color_tab_hovered;
        colors[ImGuiCol_HeaderActive]          = color_tab_active;
        // [Separator]
        colors[ImGuiCol_Separator]             = color_border;
        colors[ImGuiCol_SeparatorHovered]      = color_primary_hover;
        colors[ImGuiCol_SeparatorActive]       = color_primary_active;
        // [Resize Grip]
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 0.00);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00);
        // [Tab]
        colors[ImGuiCol_Tab]                   = color_bg;
        colors[ImGuiCol_TabHovered]            = color_primary_hover;
        colors[ImGuiCol_TabSelected]           = color_tab_active;
        colors[ImGuiCol_TabSelectedOverline]   = color_transparent;
        colors[ImGuiCol_TabDimmed]             = color_bg;
        colors[ImGuiCol_TabDimmedSelected]     = color_tab_active;
        // [Plot]
        colors[ImGuiCol_PlotLines]             = color_primary;
        colors[ImGuiCol_PlotLinesHovered]      = color_primary_hover;
        colors[ImGuiCol_PlotHistogram]         = color_primary;
        colors[ImGuiCol_PlotHistogramHovered]  = color_primary_hover;
        // [Table]
        colors[ImGuiCol_TableHeaderBg]         = ImColor(230, 230, 232);
        colors[ImGuiCol_TableBorderStrong]     = color_border;
        colors[ImGuiCol_TableBorderLight]      = ImColor(225, 225, 228);
        colors[ImGuiCol_TableRowBg]            = color_transparent;
        colors[ImGuiCol_TableRowBgAlt]         = ImColor(242, 242, 244);
        // [Nav]
        colors[ImGuiCol_NavCursor];
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.46, 0.56, 0.66, 1.00);
        colors[ImGuiCol_NavWindowingHighlight] = color_green;
        colors[ImGuiCol_NavWindowingDimBg]     = color_red;
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80, 0.80, 0.80, 0.35);
        // [Docking]
        colors[ImGuiCol_DockingPreview]        = color_primary_active;
        colors[ImGuiCol_DockingEmptyBg]        = color_primary_hover;

    }

    void theme_dark()
    {
        if (ImGui::theme_id != 1) return;
        auto& colors = ImGui::GetStyle().Colors;

        ImVec4 color_bg                 = ImColor(24, 27, 34);
        ImVec4 color_surf               = ImColor(29, 33, 41);
        ImVec4 color_surf_variant       = ImColor(36, 41, 51);


        ImVec4 color_primary            = ImColor(93, 105, 240);
        ImVec4 color_primary_hover      = ImColor(73, 85, 185);
        ImVec4 color_primary_active     = ImColor(63, 74, 162);


        ImVec4 color_frame_hover        = ImColor(46, 52, 64);
        ImVec4 color_frame_active       = ImColor(55, 62, 77);

        ImVec4 color_text               = ImColor(225, 229, 237);
        ImVec4 color_text_disabled      = ImColor(139, 148, 165);
        ImVec4 color_transparent        = ImColor(0, 0, 0, 0);


        ImVec4 theme_tab_focused        = ImColor(36, 41, 51);
        ImVec4 theme_tab_active         = ImColor(93, 105, 240);

        ImVec4 color_border             = ImColor(57, 64, 78);

        ImVec4 color_scrollbar          = ImColor(69, 78, 95);
        ImVec4 color_scrollbar_hover    = ImColor(90, 101, 121);
        ImVec4 color_scrollbar_active   = ImColor(113, 125, 147);


        // ---------------------------------------------------------------
        // ImGui 색상 적용
        // ---------------------------------------------------------------
        // [Text]
        colors[ImGuiCol_Text]                  = color_text;
        colors[ImGuiCol_TextDisabled]          = color_text_disabled;
        colors[ImGuiCol_TextSelectedBg]        = color_primary;
        colors[ImGuiCol_DragDropTarget]        = color_primary_active;
        // [Background]
        colors[ImGuiCol_WindowBg]              = color_bg;
        colors[ImGuiCol_ChildBg]               = color_surf;
        colors[ImGuiCol_PopupBg]               = color_surf;
        colors[ImGuiCol_MenuBarBg]             = color_surf;
        // [Border]
        colors[ImGuiCol_Border]                = color_border;
        colors[ImGuiCol_BorderShadow]          = color_transparent;
        // [Frame]
        colors[ImGuiCol_FrameBg]               = color_surf_variant;
        colors[ImGuiCol_FrameBgHovered]        = color_frame_hover;
        colors[ImGuiCol_FrameBgActive]         = color_frame_active;
        // [Title]
        colors[ImGuiCol_TitleBg]               = color_bg;
        colors[ImGuiCol_TitleBgActive]         = color_bg;
        colors[ImGuiCol_TitleBgCollapsed]      = color_bg;
        // [Scrollbar]
        colors[ImGuiCol_ScrollbarBg]           = color_transparent;
        colors[ImGuiCol_ScrollbarGrab]         = color_scrollbar;
        colors[ImGuiCol_ScrollbarGrabHovered]  = color_scrollbar_hover;
        colors[ImGuiCol_ScrollbarGrabActive]   = color_scrollbar_active;
        // [Checkbox]
        colors[ImGuiCol_CheckMark]             = color_primary;
        // [Slider]
        colors[ImGuiCol_SliderGrab]            = color_primary;
        colors[ImGuiCol_SliderGrabActive]      = color_primary_hover;
        // [Button]
        colors[ImGuiCol_Button]                = color_primary;
        colors[ImGuiCol_ButtonHovered]         = color_primary_hover;
        colors[ImGuiCol_ButtonActive]          = color_primary_active;
        // [Header]
        colors[ImGuiCol_Header]                = theme_tab_focused;
        colors[ImGuiCol_HeaderHovered]         = color_primary_hover;
        colors[ImGuiCol_HeaderActive]          = color_primary_active;
        // [Separator]
        colors[ImGuiCol_Separator]             = color_border;
        colors[ImGuiCol_SeparatorHovered]      = color_primary_hover;
        colors[ImGuiCol_SeparatorActive]       = color_primary_active;
        // [Resize Grip]
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 0.00);
        colors[ImGuiCol_ResizeGripHovered]     = color_scrollbar_hover;
        colors[ImGuiCol_ResizeGripActive]      = color_scrollbar_active;
        // [Tab]
        colors[ImGuiCol_Tab]                   = color_bg;
        colors[ImGuiCol_TabHovered]            = color_primary_hover;
        colors[ImGuiCol_TabSelected]           = theme_tab_active;
        colors[ImGuiCol_TabSelectedOverline]   = color_transparent;
        colors[ImGuiCol_TabDimmed]             = color_bg;
        colors[ImGuiCol_TabDimmedSelected]     = theme_tab_active;
        // [Plot]
        colors[ImGuiCol_PlotLines]             = color_primary;
        colors[ImGuiCol_PlotLinesHovered]      = color_primary_hover;
        colors[ImGuiCol_PlotHistogram]         = color_primary;
        colors[ImGuiCol_PlotHistogramHovered]  = color_primary_hover;
        // [Table]
        colors[ImGuiCol_TableHeaderBg]         = ImColor(34, 39, 49);
        colors[ImGuiCol_TableBorderStrong]     = color_border;
        colors[ImGuiCol_TableBorderLight]      = ImColor(43, 49, 61);
        colors[ImGuiCol_TableRowBg]            = color_transparent;
        colors[ImGuiCol_TableRowBgAlt]         = ImColor(255, 255, 255, 7);
        // [Nav]
        colors[ImGuiCol_NavCursor];
        colors[ImGuiCol_NavHighlight]          = ImColor(166, 180, 209);
        colors[ImGuiCol_NavWindowingHighlight] = ImColor(206, 215, 234, 179);
        colors[ImGuiCol_NavWindowingDimBg]     = ImColor(10, 13, 20, 153);
        colors[ImGuiCol_ModalWindowDimBg]      = ImColor(10, 13, 20, 166);
        // [Docking]
        colors[ImGuiCol_DockingPreview]        = color_primary_active;
        colors[ImGuiCol_DockingEmptyBg]        = color_primary_hover;
    }
}