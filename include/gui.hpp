#pragma once

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// ImGui 핵심 헤더
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot3d.h"
#include "implot.h"

// Raylib의 내부 GLFW 윈도우 접근용
#include <GLFW/glfw3.h>

#include <string>
#include <functional>

#include "icon.h"
#include "src/icon.cpp"
#include "src/font.cpp"

#include "gui_log.h"
#include "imgui_internal.h"
#include "gui_notify.hpp"
#include "gui_toggle.hpp"

namespace ImRay
{
    std::string config_path = "imgui.ini";


    // 뷰포트 상태
    RenderTexture2D view_texture; // 렌더링된 이미지가 저장되는 텍스처
    Vector2 viewport_mouse_pos = { 0.0f, 0.0f };
    bool is_viewport_hovered = false;


    // 윈도우 속성
    std::string WINDOW_TITLE;
    const float TITLEBAR_HEIGHT     = 32.0f;
    const float VIEWPORT_INTERNAL_W = 1280.0f;
    const float VIEWPORT_INTERNAL_H = 720.0f;


    // 독 스페이스 & 타이틀바 상태
    bool is_resizing = false;
    bool is_dragging_title_bar = false;
    Vector2 drag_offset = { 0.0f, 0.0f };


    // 폰트 설정
    const float  FONT_SIZE   = 18;
    const float  ICON_SIZE   = 28;
    const ImVec2 ICON_OFFSET = ImVec2(0, 6);


    // 3d 카메라 정의
    Camera camera = {
        { 1.0, -2.0, 1.0 },      // position
        { 0.0, 0.0, 0.0 },       // target
        { 0.0, 0.0, 1.0 },       // up
        45.0f,                   // fovy
        CAMERA_PERSPECTIVE       // projection
    };


    // 상태 플래그
    bool show_3d_viewport = false; // 3d 뷰포트 보이기 여부
    bool should_close_app = false; // 앱 닫기 여부
    bool show_log_window  = true;  // 로거 표시 여부



    // log window
    ImGuiLogWindow loggr;
}



namespace ImGui
{
    // 시스템 제어
    void init(const char* title, int width = 1280, int height = 720);
    void destroy();
    bool is_running();

    // 통합 컨텍스트 렌더링 함수
    void context(std::function<void()> func);

    // 설정 함수
    void load_config(const std::string& path);

    // 뷰포트 마우스 및 상태 유틸리티
    Vector2 get_viewport_mouse_pos(); // 뷰포트의 마우스 위치
    bool    is_viewport_hovered();



    // =====================================================
    // --- 설정 핸들러 콜백 함수 시작 ---
    // =====================================================
    static void* SettingsHandler_ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
    {
        return (void*)1;
    }

    static void SettingsHandler_ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line)
    {
        // ini 파일에서 우리가 정의한 섹션의 각 줄을 읽어올 때 호출
        int val;
        if (sscanf(line, "ShowLogWindow=%d", &val) == 1) {
            ImRay::show_log_window = (val != 0);
        }
        else if (sscanf(line, "Show3DViewport=%d", &val) == 1) {
            ImRay::show_3d_viewport = (val != 0);
        }
    }

    static void SettingsHandler_WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
    {
        // 프로그램 종료 시 또는 ini 파일 저장 시 호출
        buf->appendf("[%s][Main]\n", handler->TypeName);
        buf->appendf("ShowLogWindow=%d\n", ImRay::show_log_window ? 1 : 0);
        buf->appendf("Show3DViewport=%d\n", ImRay::show_3d_viewport ? 1 : 0);
        buf->appendf("\n");
    }
    // --- 설정 핸들러 콜백 함수 끝 ---




    // =====================================================
    // --- 토글 버튼 시작 ---
    // =====================================================
    void ToggleButton(const char* str_id, bool* v);
    // --- 토글 버튼 끝 ---
}


inline void DrawWorldAxesThick(float length, float thickness)
{
    Vector3 origin = { 0.0f, 0.0f, 0.0f };

    // 원기둥의 면 개수
    int sides = 8;

    // X축 (빨강)
    DrawCylinderEx(origin, (Vector3){ length, 0.0f, 0.0f }, thickness, thickness, sides, RED);

    // Y축 (녹색)
    DrawCylinderEx(origin, (Vector3){ 0.0f, length, 0.0f }, thickness, thickness, sides, GREEN);

    // Z축 (파랑)
    DrawCylinderEx(origin, (Vector3){ 0.0f, 0.0f, length }, thickness, thickness, sides, BLUE);
}

namespace ImGui
{
    void init(const char* title, int width, int height)
    {
        ImRay::WINDOW_TITLE = title;

        // ---------------------------------------------------------------
        // ImGui & raylib 초기화
        // ---------------------------------------------------------------
        SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE ); // FLAG_WINDOW_TRANSPARENT
        InitWindow(width, height, title);
        SetTargetFPS(60);
        SetExitKey(0); // esc로 인한 종료 방지

        // imgui 컨텍스트 생성
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImPlot3D::CreateContext();

        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "ImRaySettings"; // ini 파일에 기록될 섹션 이름
        ini_handler.TypeHash = ImHashStr("ImRaySettings");
        ini_handler.ReadOpenFn = SettingsHandler_ReadOpen;
        ini_handler.ReadLineFn = SettingsHandler_ReadLine;
        ini_handler.WriteAllFn = SettingsHandler_WriteAll;
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);

        // 플래그 설정
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 네비게이션 활성화
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 도킹 활성화
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // 멀티 뷰포트

        // config 파일 저장할 경로 설정
        io.IniFilename = ImRay::config_path.c_str();


        // ---------------------------------------------------------------
        // ImGui Ui 색상 & 모양 스타일 설정
        // ---------------------------------------------------------------
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                  = ImVec4(0.92, 0.93, 0.94, 1.00); // 가독성을 위한 밝은 회색 Text
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50, 0.52, 0.54, 1.00); // 비활성화된 Text를 위한 옅은 회색
        colors[ImGuiCol_WindowBg]              = ImVec4(0.14, 0.14, 0.16, 0.7); // 약간 푸른빛이 도는 어두운 Background
        colors[ImGuiCol_ChildBg]               = ImVec4(0.16, 0.16, 0.18, 1.00); // Child 요소를 위한 약간 더 밝은 색상
        colors[ImGuiCol_PopupBg]               = ImVec4(0.18, 0.18, 0.20, 1.00); // Popup Background
        colors[ImGuiCol_Border]                = ImVec4(0.28, 0.29, 0.30, 0.60); // 부드러운 Border 색상
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00, 0.00, 0.00, 0.00); // Border Shadow 없음
        colors[ImGuiCol_FrameBg]               = ImVec4(0.20, 0.22, 0.24, 1.00); // Frame Background
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22, 0.24, 0.26, 1.00); // Frame Hover 효과
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.24, 0.26, 0.28, 1.00); // Active Frame Background
        colors[ImGuiCol_TitleBg]               = ImVec4(0.14, 0.14, 0.16, 1.00); // Title Background
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16, 0.16, 0.18, 1.00); // Active Title Background
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.14, 0.14, 0.16, 1.00); // Collapsed Title Background
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20, 0.20, 0.22, 1.00); // Menu Bar Background
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.16, 0.16, 0.18, 1.00); // Scrollbar Background
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.24, 0.26, 0.28, 1.00); // Scrollbar Grab을 위한 어두운 강조색
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28, 0.30, 0.32, 1.00); // Scrollbar Grab Hover
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.32, 0.34, 0.36, 1.00); // Scrollbar Grab Active
        colors[ImGuiCol_CheckMark]             = ImVec4(0.46, 0.56, 0.66, 1.00); // 짙은 파란색 Checkmark
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.36, 0.46, 0.56, 1.00); // 짙은 파란색 Slider Grab
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.40, 0.50, 0.60, 1.00); // Active Slider Grab
        colors[ImGuiCol_Button]                = ImVec4(0.24, 0.34, 0.44, 1.00); // 짙은 파란색 Button
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.28, 0.38, 0.48, 1.00); // Button Hover 효과
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.32, 0.42, 0.52, 1.00); // Active Button
        colors[ImGuiCol_Header]                = ImVec4(0.24, 0.34, 0.44, 1.00); // Button과 비슷한 Header 색상
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.28, 0.38, 0.48, 1.00); // Header Hover 효과
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.32, 0.42, 0.52, 1.00); // Active Header
        colors[ImGuiCol_Separator]             = ImVec4(0.28, 0.29, 0.30, 1.00); // Separator 색상
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.46, 0.56, 0.66, 1.00); // Separator Hover 효과
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.46, 0.56, 0.66, 1.00); // Active Separator
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 1.00); // Resize Grip
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00); // Resize Grip Hover 효과
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00); // Active Resize Grip
        colors[ImGuiCol_Tab]                   = ImVec4(0.20, 0.22, 0.24, 1.00); // 비활성 Tab
        colors[ImGuiCol_TabHovered]            = ImVec4(0.28, 0.38, 0.48, 1.00); // Tab Hover 효과
        colors[ImGuiCol_TabActive]             = ImVec4(0.24, 0.34, 0.44, 1.00); // Active Tab 색상
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.20, 0.22, 0.24, 1.00); // 포커스를 잃은(Unfocused) Tab
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.24, 0.34, 0.44, 1.00); // Active 상태지만 포커스를 잃은 Tab
        colors[ImGuiCol_PlotLines]             = ImVec4(0.46, 0.56, 0.66, 1.00); // Plot Lines
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.46, 0.56, 0.66, 1.00); // Plot Lines Hover 효과
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.36, 0.46, 0.56, 1.00); // Histogram 색상
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.40, 0.50, 0.60, 1.00); // Histogram Hover 효과
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20, 0.22, 0.24, 1.00); // Table Header Background
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.28, 0.29, 0.30, 1.00); // Table을 위한 짙은 Border
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.24, 0.25, 0.26, 1.00); // Table을 위한 옅은 Border
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.20, 0.22, 0.24, 1.00); // Table Row Background
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.22, 0.24, 0.26, 1.00); // 교차(Alternate) Row Background
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.24, 0.34, 0.44, 0.35); // 선택된 Text Background
        colors[ImGuiCol_DragDropTarget]        = ImVec4(0.46, 0.56, 0.66, 0.90); // Drag and Drop Target
        colors[ImGuiCol_NavHighlight]          = ImVec4(0.46, 0.56, 0.66, 1.00); // Navigation Highlight
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00, 1.00, 1.00, 0.70); // Windowing Highlight
        colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80, 0.80, 0.80, 0.20); // Windowing을 위한 어두운(Dim) Background
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80, 0.80, 0.80, 0.35); // Modal Window를 위한 어두운(Dim) Background

        // Style adjustments
        style.WindowPadding     = ImVec2(8.00, 8.00); // Window 내측 여백 (Padding)
        style.FramePadding      = ImVec2(5.00, 2.00); // Frame 내측 여백 (Padding)
        style.CellPadding       = ImVec2(6.00, 6.00); // Table Cell 내측 여백 (Padding)
        style.ItemSpacing       = ImVec2(6.00, 6.00); // Item 간의 간격 (Spacing)
        style.ItemInnerSpacing  = ImVec2(6.00, 6.00); // Item 내부 요소 간의 간격 (Inner Spacing)
        style.TouchExtraPadding = ImVec2(0.00, 0.00); // 터치 조작을 위한 추가 여백 (Padding)
        style.IndentSpacing     = 25;                 // 들여쓰기 (Indent) 간격
        style.ScrollbarSize     = 11;                 // Scrollbar 두께/크기
        style.GrabMinSize       = 10;                 // 슬라이더 등 Grab(손잡이)의 최소 크기
        style.WindowBorderSize  = 1;                  // Window 테두리 (Border) 두께
        style.ChildBorderSize   = 1;                  // Child 창 테두리 (Border) 두께
        style.PopupBorderSize   = 1;                  // Popup 테두리 (Border) 두께
        style.FrameBorderSize   = 1;                  // Frame 테두리 (Border) 두께
        style.TabBorderSize     = 1;                  // Tab 테두리 (Border) 두께
        style.WindowRounding    = 3;                  // Window 모서리 둥글기 (Rounding)
        style.ChildRounding     = 4;                  // Child 창 모서리 둥글기 (Rounding)
        style.FrameRounding     = 3;                  // Frame 모서리 둥글기 (Rounding)
        style.PopupRounding     = 4;                  // Popup 모서리 둥글기 (Rounding)
        style.ScrollbarRounding = 9;                  // Scrollbar 모서리 둥글기 (Rounding)
        style.GrabRounding      = 3;                  // Grab(손잡이) 모서리 둥글기 (Rounding)
        style.LogSliderDeadzone = 4;                  // Logarithmic Slider의 데드존(Deadzone) 크기
        style.TabRounding       = 4;                  // Tab 모서리 둥글기 (Rounding)


        // ---------------------------------------------------------------
        // 폰트 설정
        // ---------------------------------------------------------------
        ImFontConfig config;
        config.MergeMode = true;
        config.GlyphOffset = ImRay::ICON_OFFSET;
        config.GlyphMinAdvanceX = ImRay::FONT_SIZE;
        static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };

        // 글자 폰트 추가
        io.Fonts->Clear();
        io.Fonts->AddFontFromMemoryCompressedTTF(
            NEXON_Lv2_Gothic_Medium_compressed_data,
            NEXON_Lv2_Gothic_Medium_compressed_size,
            ImRay::FONT_SIZE,
            NULL,
            io.Fonts->GetGlyphRangesKorean()
        );

        // 아이콘 폰트 추가
        io.Fonts->AddFontFromMemoryCompressedTTF(
            MaterialSymbolsRounded_compressed_data,
            MaterialSymbolsRounded_compressed_size,
            ImRay::ICON_SIZE,
            &config,
            icon_ranges
        );


        // ---------------------------------------------------------------
        // 윈도우 및 뷰포트 텍스쳐 생성
        // ---------------------------------------------------------------
        GLFWwindow* window = glfwGetCurrentContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        ImRay::view_texture = LoadRenderTexture((int) ImRay::VIEWPORT_INTERNAL_W, (int) ImRay::VIEWPORT_INTERNAL_H);
    }

    void destroy()
    {
        UnloadRenderTexture(ImRay::view_texture);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImPlot3D::DestroyContext();
        ImGui::DestroyContext();
        CloseWindow();
    }

    bool is_running()
    {
        return !WindowShouldClose() && !ImRay::should_close_app;
    }


    void    load_config(const std::string& path)     { ImRay::config_path = path;         }
    Vector2 get_viewport_mouse_pos()                 { return ImRay::viewport_mouse_pos;  }
    bool    is_viewport_hovered()                    { return ImRay::is_viewport_hovered; }


    void context(std::function<void()> func)
    {
        // ---------------------------------------------------------------
        // 1. ImGui 프레임 시작
        // ---------------------------------------------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        int currentWidth = GetScreenWidth();
        int currentHeight = GetScreenHeight();


        // ---------------------------------------------------------------
        // 2. 우측 하단 크기 조절 (Resizing)
        // ---------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();
        Rectangle resizeGripArea = { (float)currentWidth - 15, (float)currentHeight - 15, 15, 15 };

        if (CheckCollisionPointRec(mousePos, resizeGripArea)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ImRay::is_resizing = true;
        }

        if (ImRay::is_resizing) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                int newW = (int)mousePos.x;
                int newH = (int)mousePos.y;
                if (newW < 800) newW = 800;
                if (newH < 600) newH = 600;
                ::SetWindowSize(newW, newH);
            } else {
                ImRay::is_resizing = false;
            }
        }

        ImVec2 p1 = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
        ImVec2 p2 = ImVec2(viewport->Pos.x + viewport->Size.x - 15, viewport->Pos.y + viewport->Size.y);
        ImVec2 p3 = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y - 15);
        ImGui::GetForegroundDrawList()->AddTriangleFilled(p1, p2, p3, IM_COL32(150, 150, 150, 255));


        // ---------------------------------------------------------------
        // 3. 타이틀 바
        // ---------------------------------------------------------------
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImRay::TITLEBAR_HEIGHT));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags titleFlags = ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
        ImGui::Begin("CustomTitleBar", nullptr, titleFlags);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            ImRay::is_dragging_title_bar = true;

            ImVec2 mousePos = ImGui::GetMousePos();
            Vector2 winPos = GetWindowPosition();

            ImRay::drag_offset.x = mousePos.x - winPos.x;
            ImRay::drag_offset.y = mousePos.y - winPos.y;
        }

        if (ImRay::is_dragging_title_bar) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 currentAbsoluteMouse = ImGui::GetMousePos();
                ::SetWindowPosition((int)(currentAbsoluteMouse.x - ImRay::drag_offset.x),
                                    (int)(currentAbsoluteMouse.y - ImRay::drag_offset.y));
            } else {
                ImRay::is_dragging_title_bar = false;
            }
        }

        float textWidth = ImGui::CalcTextSize(ImRay::WINDOW_TITLE.c_str()).x;
        ImGui::SetCursorPosX((viewport->Size.x - textWidth) * 0.5f);
        ImGui::SetCursorPosY((ImRay::TITLEBAR_HEIGHT - ImGui::GetFontSize()) * 0.5f);
        ImGui::Text("%s", ImRay::WINDOW_TITLE.c_str());

        // ---------------------------------------------------------------
        // 3-1. 타이틀바 버튼
        // ---------------------------------------------------------------
        // 버튼 위치 설정
        float closeBtnWidth = 40.0f;
        float maxBtnWidth = 40.0f; // 최대화 버튼 너비 추가
        float settingsBtnWidth = 45.0f;

        // --- 설정 버튼 ---
        // 최대화 버튼이 추가되었으므로 위치를 그만큼 왼쪽으로 밀어줍니다.
        ImGui::SameLine(viewport->Size.x - closeBtnWidth - maxBtnWidth - settingsBtnWidth);
        ImGui::SetCursorPosY(0.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        if (ImGui::Button(ICON_MD_SETTINGS, ImVec2(settingsBtnWidth, ImRay::TITLEBAR_HEIGHT))) {
            ImGui::OpenPopup("SettingsPopup");
        }
        ImGui::PopStyleColor(3);

        // 설정 팝업 정의 (팝업 위치도 동일하게 조정)
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - closeBtnWidth - maxBtnWidth - settingsBtnWidth, viewport->Pos.y + ImRay::TITLEBAR_HEIGHT));
        if (ImGui::BeginPopup("SettingsPopup")) {
            ImGui::Text("설정 메뉴");
            ImGui::Separator();
            ImGui::Checkbox("3D 뷰어", &ImRay::show_3d_viewport);
            ImGui::Checkbox("로그", &ImRay::show_log_window);
            ImGui::EndPopup();
        }

        // --- 최대화/이전 크기로 복원 버튼 ---
        ImGui::SameLine(viewport->Size.x - closeBtnWidth - maxBtnWidth);
        ImGui::SetCursorPosY(0.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        // 창 상태에 따라 아이콘 텍스트 분기 처리
        const char* maxIcon = IsWindowMaximized() ? ICON_MD_FULLSCREEN_EXIT : ICON_MD_FULLSCREEN;

        if (ImGui::Button(maxIcon, ImVec2(maxBtnWidth, ImRay::TITLEBAR_HEIGHT))) {
            if (IsWindowMaximized()) {
                RestoreWindow();  // 최대화 상태라면 원래 크기로 복원
            } else {
                MaximizeWindow(); // 일반 상태라면 최대화
            }
        }
        ImGui::PopStyleColor(3);

        // --- 닫기(X) 버튼 ---
        ImGui::SameLine(viewport->Size.x - closeBtnWidth);
        ImGui::SetCursorPosY(0.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(ICON_MD_CLOSE, ImVec2(closeBtnWidth, ImRay::TITLEBAR_HEIGHT))) {
            ImRay::should_close_app = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();


        // ---------------------------------------------------------------
        // 4. 독스페이스
        // ---------------------------------------------------------------
        ImVec2 dockPos = ImVec2(viewport->Pos.x, viewport->Pos.y + ImRay::TITLEBAR_HEIGHT);
        ImVec2 dockSize = ImVec2(viewport->Size.x, viewport->Size.y - ImRay::TITLEBAR_HEIGHT);

        ImGui::SetNextWindowPos(dockPos);
        ImGui::SetNextWindowSize(dockSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags dockFlags = ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MainRootDockSpaceWindow", nullptr, dockFlags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainRootDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();




        // ---------------------------------------------------------------
        // 5. 렌더링된 텍스처를 담을 뷰포트 창 띄우기
        // ---------------------------------------------------------------
        if (ImRay::show_3d_viewport)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            // Begin의 리턴값을 받되, End()는 if문 밖에서 무조건 호출해야 합니다.
            bool is_viewport_visible = ImGui::Begin("3D Viewport", &ImRay::show_3d_viewport);

            if (is_viewport_visible)
            {
                ImVec2 availSize = ImGui::GetContentRegionAvail();
                float targetAspect = ImRay::VIEWPORT_INTERNAL_W / ImRay::VIEWPORT_INTERNAL_H;
                float availAspect = availSize.x / availSize.y;

                ImVec2 imageSize;
                if (availAspect > targetAspect) {
                    imageSize.y = availSize.y;
                    imageSize.x = imageSize.y * targetAspect;
                } else {
                    imageSize.x = availSize.x;
                    imageSize.y = imageSize.x / targetAspect;
                }

                ImVec2 cursorStartPos = ImGui::GetCursorScreenPos();
                float offsetX = (availSize.x - imageSize.x) * 0.5f;
                float offsetY = (availSize.y - imageSize.y) * 0.5f;
                ImVec2 imagePos = ImVec2(cursorStartPos.x + offsetX, cursorStartPos.y + offsetY);
                ImGui::SetCursorScreenPos(imagePos);

                ImTextureID tex_id = (ImTextureID)(intptr_t) ImRay::view_texture.texture.id;
                ImGui::Image(tex_id, imageSize, ImVec2(0, 1), ImVec2(1, 0));

                ImRay::is_viewport_hovered = ImGui::IsItemHovered();
                ImVec2 imMousePosGlobal = ImGui::GetMousePos();
                ImRay::viewport_mouse_pos.x = (imMousePosGlobal.x - imagePos.x) * (ImRay::VIEWPORT_INTERNAL_W / imageSize.x);
                ImRay::viewport_mouse_pos.y = (imMousePosGlobal.y - imagePos.y) * (ImRay::VIEWPORT_INTERNAL_H / imageSize.y);

                // ---------------------------------------------------------------
                // 6. 사용자 콘텐츠 렌더링 (Raylib 텍스처 + 사용자 ImGui UI)
                // ---------------------------------------------------------------
                BeginTextureMode(ImRay::view_texture);
                ClearBackground(BLANK);

                // 뷰포트 조작시 마우스 커서 숨김
                if (ImGui::is_viewport_hovered()) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        if (IsCursorHidden()) EnableCursor();
                        else DisableCursor();
                    }
                }

                if (IsCursorHidden()) {
                    UpdateCamera(&ImRay::camera, CAMERA_FREE);
                }

                BeginMode3D(ImRay::camera);
                DrawWorldAxesThick(0.5, 0.01);
                rlPushMatrix();
                rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                DrawGrid(20, 1.0f);
                rlPopMatrix();
                EndMode3D();
                ImGui::PopStyleVar();

                // 사용자 람다 콜백 실행
                if (func) {
                    func();
                }

                EndTextureMode();

            }
            else
            {
                // 창이 접혀있을 때도 사용자가 정의한 다른 ImGui 창들이 렌더링되도록 콜백 유지
                if (func) func();
            }

            ImGui::End();
        }
        else
        {
            // 3D 뷰포트가 아예 꺼져있을 때도 사용자 UI 루프가 돌도록 처리
            if (func) func();
        }

        // ---------------------------------------------------------------
        // ImGui log 렌더링
        // ---------------------------------------------------------------
        if (ImRay::show_log_window)
            ImRay::loggr.draw("로그", &ImRay::show_log_window);


        // ---------------------------------------------------------------
        // ImGui notiy 렌더링
        // ---------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
        ImGui::RenderNotifications();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(1);


        // ---------------------------------------------------------------
        // 7. 메인 화면 최종 출력 및 ImGui 렌더링
        // ---------------------------------------------------------------
        ImGui::Render();

        BeginDrawing();
            ClearBackground(BLANK);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        EndDrawing();
    }
}