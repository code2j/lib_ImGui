#include "ui.hpp"
#include "icon.cpp"
#include "ui_font_level2.cpp"
#include "ui_font_d2coding.cpp"


namespace 
{
std::string config_path = "imgui.ini";

// ==================================================
// 뷰포트 상태
// ==================================================
RenderTexture2D view_texture; // 렌더링된 이미지가 저장되는 텍스처
Vector2 viewport_mouse_pos = { 0.0, 0.0 };
bool is_viewport_hovered   = false;


// ==================================================
// 윈도우 속성
// ==================================================
std::string WINDOW_TITLE;
const float WINDOW_WIDTH    = 1280.0;
const float WINDOW_HEIGHT   = 720.0;


// ==================================================
// 독 스페이스 & 타이틀바 상태
// ==================================================
const float TITLEBAR_HEIGHT = 34.0;
bool is_resizing            = false;
bool is_dragging_title_bar  = false;
Vector2 drag_offset         = { 0.0, 0.0 };


// ==================================================
// 폰트 설정
// ==================================================
const float  FONT_SIZE   = 20;
const float  ICON_SIZE   = 28;
const ImVec2 ICON_OFFSET = ImVec2(0, 6);


// ==================================================
// 3d 카메라 정의
// ==================================================
Camera camera = {
    { 1.0, 0.5, 2.0 },      // position
    { 0.0, 0.0, 0.0 },       // target
    { 0.0, 1.0, 0.0 },       // up
    45.0f,                   // fovy
    CAMERA_PERSPECTIVE       // projection
};





// ==================================================
// 로그 윈도우
// ==================================================
ImGuiLogger loggr;


// ==================================================
// 스카이 박스
// ==================================================
Model skybox;


}




namespace ImGui
{


void init(const char* title, int width, int height)
{
    WINDOW_TITLE = title;

    // ---------------------------------------------------------------
    // ImGui & raylib 초기화
    // ---------------------------------------------------------------
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT ); // FLAG_WINDOW_TRANSPARENT
    InitWindow(width, height, title);
    SetTargetFPS(60);
    SetExitKey(0); // esc로 인한 종료 방지

    // imgui 컨텍스트 생성
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImPlot3D::CreateContext();

    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName   = ""; // ini 파일에 기록될 섹션 이름
    ini_handler.TypeHash   = ImHashStr("");
    ini_handler.ReadOpenFn = read_open;
    ini_handler.ReadLineFn = read_line;
    ini_handler.WriteAllFn = write_all;
    ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);

    // 플래그 설정
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 네비게이션 활성화
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 도킹 활성화
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // 멀티 뷰포트

    // config 파일 저장할 경로 설정
    io.IniFilename = config_path.c_str();


    // ---------------------------------------------------------------
    // ImGui Ui 색상 & 모양 스타일 설정
    // ---------------------------------------------------------------
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.92, 0.93, 0.94, 1.00); // 가독성을 위한 밝은 회색 Text
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50, 0.52, 0.54, 1.00); // 비활성화된 Text를 위한 옅은 회색
    colors[ImGuiCol_WindowBg]              = ImVec4(0.14, 0.14, 0.16, 0.50); // 약간 푸른빛이 도는 어두운 Background
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
    colors[ImGuiCol_TabSelectedOverline]   = ImVec4(0.00, 0.00, 0.00, 0.00);




    // Style adjustments
    style.WindowPadding     = ImVec2(8.00, 8.00); // Window 내측 여백 (Padding)
    style.FramePadding      = ImVec2(5.00, 6.00); // Frame 내측 여백 (Padding)
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
    style.TabBorderSize     = 0;                  // Tab 테두리 (Border) 두께
    style.WindowRounding    = 3;                  // Window 모서리 둥글기 (Rounding)
    style.ChildRounding     = 4;                  // Child 창 모서리 둥글기 (Rounding)
    style.FrameRounding     = 3;                  // Frame 모서리 둥글기 (Rounding)
    style.PopupRounding     = 4;                  // Popup 모서리 둥글기 (Rounding)
    style.ScrollbarRounding = 9;                  // Scrollbar 모서리 둥글기 (Rounding)
    style.GrabRounding      = 3;                  // Grab(손잡이) 모서리 둥글기 (Rounding)
    style.LogSliderDeadzone = 4;                  // Logarithmic Slider의 데드존(Deadzone) 크기
    style.TabRounding       = 6;                  // Tab 모서리 둥글기 (Rounding)
    style.TabBarBorderSize  = 0;

    style.WindowMenuButtonPosition = ImGuiDir_None; // 탭 최소화 버튼 제거


    // ---------------------------------------------------------------
    // 폰트 설정
    // ---------------------------------------------------------------
    ImFontConfig config;
    config.MergeMode        = true;
    config.GlyphOffset      = ICON_OFFSET;
    config.GlyphMinAdvanceX = FONT_SIZE;
    static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };

    // 글자 폰트 추가
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryCompressedTTF(
        compressed_data,
        compressed_size,
        FONT_SIZE,
        NULL,
        io.Fonts->GetGlyphRangesKorean()
    );

    // 아이콘 폰트 추가
    io.Fonts->AddFontFromMemoryCompressedTTF(
        MaterialSymbolsRounded_compressed_data,
        MaterialSymbolsRounded_compressed_size,
        ICON_SIZE,
        &config,
        icon_ranges
    );

    // 로거 전용 폰트 로딩
    ImGuiExt::D2Cording = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        font2_compressed_data,
        font2_compressed_size,
        FONT_SIZE,
        NULL,
        ImGui::GetIO().Fonts->GetGlyphRangesKorean()
    );



    // ---------------------------------------------------------------
    // 스카이 박스
    // ---------------------------------------------------------------
#if(SKYBOX_ON)
    skybox = load_skybox(
        IMGUI_ROOT "/data/shaders/skybox.vs",
        IMGUI_ROOT "/data/shaders/skybox.fs",
        IMGUI_ROOT "/data/textures/skybox/skybox.png"
    );
#endif


    // ---------------------------------------------------------------
    // 윈도우 및 뷰포트 텍스쳐 생성
    // ---------------------------------------------------------------
    GLFWwindow* window = glfwGetCurrentContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    view_texture = LoadRenderTexture((int)WINDOW_WIDTH, (int)WINDOW_HEIGHT);
}

void destroy()
{
#if(SKYBOX_ON)
    // 스카이 박스 해제
    UnloadShader(skybox.materials[0].shader);
    UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    UnloadModel(skybox);
#endif

    UnloadRenderTexture(::view_texture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImPlot3D::DestroyContext();
    ImGui::DestroyContext();
    CloseWindow();
}


bool should_close()
{
    return WindowShouldClose() || ImGuiExt::should_close_app;
}


void    load_config(const std::string& path)     { config_path = path;         }
Vector2 get_viewport_mouse_pos()                 { return viewport_mouse_pos;  }
bool    is_viewport_hovered()                    { return ::is_viewport_hovered; }


void context(std::function<void()> func)
{
    // ---------------------------------------------------------------
    // 1. ImGui 프레임 시작
    // ---------------------------------------------------------------
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    int curr_w = GetScreenWidth();
    int curr_h = GetScreenHeight();


    // ---------------------------------------------------------------
    // 2. 우측 하단 크기 조절 (Resizing)
    // ---------------------------------------------------------------
    Vector2 mousePos = GetMousePosition();
    Rectangle resizeGripArea = { (float)curr_w - 15, (float)curr_h - 15, 15, 15 };

    if (CheckCollisionPointRec(mousePos, resizeGripArea)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) is_resizing = true;
    }

    if (is_resizing) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            int newW = (int)mousePos.x;
            int newH = (int)mousePos.y;
            if (newW < 800) newW = 800;
            if (newH < 600) newH = 600;
            ::SetWindowSize(newW, newH);
        } else {
            is_resizing = false;
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
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, TITLEBAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags titleFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

    ImGui::Begin("CustomTitleBar", nullptr, titleFlags);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        is_dragging_title_bar = true;

        // ImGui의 절대 화면 마우스 좌표를 사용합니다.
        ImVec2 mouseGlobal = ImGui::GetMousePos();
        Vector2 winPos = GetWindowPosition();

        // 창의 좌상단 기준 클릭한 오프셋을 계산하여 저장합니다.
        drag_offset.x = mouseGlobal.x - winPos.x;
        drag_offset.y = mouseGlobal.y - winPos.y;
    }

    if (is_dragging_title_bar) {
        if (ImGui::IsMouseDown(0)) {
            // 드래그 중에도 절대 화면 마우스 좌표를 기준으로 창 위치를 업데이트합니다.
            ImVec2 mouseGlobal = ImGui::GetMousePos();

            // 절대 마우스 위치에서 처음 클릭했던 오프셋을 빼서 창 위치를 세팅합니다.
            ::SetWindowPosition((int)(mouseGlobal.x - drag_offset.x),
                                (int)(mouseGlobal.y - drag_offset.y));
        } else {
            is_dragging_title_bar = false;
        }
    }

    float textWidth = ImGui::CalcTextSize(WINDOW_TITLE.c_str()).x;
    ImGui::SetCursorPosX((viewport->Size.x - textWidth) * 0.5f);
    ImGui::SetCursorPosY((TITLEBAR_HEIGHT - ImGui::GetFontSize()) * 0.5f);
    ImGui::Text("%s", WINDOW_TITLE.c_str());

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

    if (ImGui::Button(ICON_MD_SETTINGS, ImVec2(settingsBtnWidth, TITLEBAR_HEIGHT))) {
        ImGui::OpenPopup("SettingsPopup");
    }
    ImGui::PopStyleColor(3);

    // 설정 팝업 정의 (팝업 위치도 동일하게 조정)
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - closeBtnWidth - maxBtnWidth - settingsBtnWidth, viewport->Pos.y + TITLEBAR_HEIGHT));
    if (ImGui::BeginPopup("SettingsPopup")) {
        ImGui::Text("설정");
        ImGui::Separator();
        ImGui::Checkbox("3D 뷰포트", & ImGuiExt::show_3d_viewport);
        ImGui::Checkbox("로그", &ImGuiExt::show_log_window);
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

    if (ImGui::Button(maxIcon, ImVec2(maxBtnWidth, ::TITLEBAR_HEIGHT))) {
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

    if (ImGui::Button(ICON_MD_CLOSE, ImVec2(closeBtnWidth, TITLEBAR_HEIGHT))) {
        ImGuiExt::should_close_app = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);


    // ---------------------------------------------------------------
    // 4. 독스페이스
    // ---------------------------------------------------------------
    const float up_offset = 1.1; // 타이틀바 라운딩으로 생긴 여백 지우기 용

    ImVec2 dockPos = ImVec2(viewport->Pos.x, viewport->Pos.y + TITLEBAR_HEIGHT-up_offset);
    ImVec2 dockSize = ImVec2(viewport->Size.x, viewport->Size.y - TITLEBAR_HEIGHT);

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
    if (ImGuiExt::show_3d_viewport)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin(" " ICON_MD_DEPLOYED_CODE " 3D 뷰포트 ");

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float targetAspect = WINDOW_WIDTH / WINDOW_HEIGHT;
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

        ImTextureID tex_id = (ImTextureID)(intptr_t) view_texture.texture.id;
        ImGui::Image(tex_id, imageSize, ImVec2(0, 1), ImVec2(1, 0));

        ::is_viewport_hovered = ImGui::IsItemHovered();
        ImVec2 imMousePosGlobal = ImGui::GetMousePos();
        viewport_mouse_pos.x = (imMousePosGlobal.x - imagePos.x) * (WINDOW_WIDTH / imageSize.x);
        viewport_mouse_pos.y = (imMousePosGlobal.y - imagePos.y) * (WINDOW_HEIGHT / imageSize.y);

        // ---------------------------------------------------------------
        // 6. 사용자 콘텐츠 렌더링 (Raylib 텍스처 + 사용자 ImGui UI)
        // ---------------------------------------------------------------
        BeginTextureMode(view_texture);
        ClearBackground(BLANK);

        // 커서가 숨겨져 있을 때 (카메라 조작 중)
        if (IsCursorHidden()) {
            // 뷰포트 호버 여부와 상관없이 우클릭으로 무조건 커서 복구
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                EnableCursor();
            }
            // 카메라 업데이트 수행
            UpdateCamera(&camera, CAMERA_FREE);
        }
        // 커서가 보일 때 (일반 UI 조작 중)
        else {
            // 뷰포트 위에 있을 때만 우클릭으로 커서 숨기기 (카메라 조작 시작)
            if (ImGui::is_viewport_hovered()) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    DisableCursor();
                }
            }
        }


        ImGui::PopStyleVar();

        // 사용자 람다 콜백 실행
        if (func) {
            BeginMode3D(camera);

#if(SKYBOX_ON)
                rlDisableBackfaceCulling();
                rlDisableDepthMask();

                DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, WHITE);

                rlEnableBackfaceCulling();
                rlEnableDepthMask();
#endif // 스카이박스 end

                // 그리드
                DrawGrid(10, 1);

                // 월드 축
                DrawWorldAxesThick(0.5, 0.01);

                // 외부 함수
                func();
            EndMode3D();
            DrawFPS(10, 10);
        }

        EndTextureMode();




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
    if (ImGuiExt::show_log_window)
        loggr.draw(" " ICON_MD_EDIT_DOCUMENT " 로그 ", &ImGuiExt::show_log_window);


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