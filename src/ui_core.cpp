#include "ui.hpp"
#include "ui_font_icon.cpp"
#include "ui_font_level2.cpp"
#include "ui_font_d2coding.cpp"

#include <filesystem>
#include <fstream>
#include <X11/Xlib.h> // 파일 상단에 추가

namespace fs = std::filesystem;

namespace
{

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



        // 플래그 설정
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 키보드 네비게이션 활성화
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 도킹 활성화
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // 멀티 뷰포트




        // ---------------------------------------------------------------
        // 테마 색상 변수 정의
        // ---------------------------------------------------------------
        if (ImGuiExt::theme_id == 0) ImGui::style_white();
        else                         ImGui::style_dark();


        ImGuiStyle& style = ImGui::GetStyle();

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
        style.WindowBorderSize  = 0;                  // Window 테두리 (Border) 두께
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
        // 안티엘리어싱 설정
        // ---------------------------------------------------------------
        style.AntiAliasedLines          = true;
        style.AntiAliasedFill           = true;
        style.AntiAliasedLinesUseTex    = true;

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
        skybox = load_skybox(IMGUI_ROOT "/data/textures/skybox/skybox.png");
    #endif


        // ---------------------------------------------------------------
        // 쉐이더 불러오기
        // ---------------------------------------------------------------
        ImGuiExt::shader_instancing = LoadShader(
            IMGUI_ROOT "/data/shaders/lighting_instancing.vs",
            IMGUI_ROOT "/data/shaders/lighting.fs");


        // ---------------------------------------------------------------
        // 쉐이더 설정
        // ---------------------------------------------------------------
        int ambientLoc = GetShaderLocation(ImGuiExt::shader_instancing, "ambient");
        float ambient[4] = { 10.0f, 10.0f, 10.0f, 10.0f }; // R, G, B, Alpha 순서
        SetShaderValue(ImGuiExt::shader_instancing, ambientLoc, ambient, SHADER_UNIFORM_VEC4);




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


    void load_config(const char* path)
    {
        fs::path p(path);

        // 디렉토리 생성
        fs::path parent_dir = p.parent_path();
        if (!parent_dir.empty() && !fs::exists(parent_dir)) {
            // [폴더가 없음]
            fs::create_directories(parent_dir); // 하위 폴더까지 한 번에 생성
        }


        // 파일 생성
        if (!fs::exists(p)) {
            // [파일이 없음]
            std::ofstream outfile(p);
            if (outfile.is_open()) {
                outfile.close();
            }
        }


        // config path 설정
        ImGui::GetIO().IniFilename = path;


        // ini 콜백 등록
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName   = ""; // ini 파일에 기록될 섹션 이름
        ini_handler.TypeHash   = ImHashStr("");
        ini_handler.ReadOpenFn = read_open;
        ini_handler.ReadLineFn = read_line;
        ini_handler.WriteAllFn = write_all;
        ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
    }

    Vector2 get_viewport_mouse_pos()                 { return viewport_mouse_pos;  }
    bool    is_viewport_hovered()                    { return ::is_viewport_hovered; }


    bool context(std::function<void()> func)
    {
        if (should_close()) {
            return false;
        }

        // ---------------------------------------------------------------
        // 1. ImGui 프레임 시작
        // ---------------------------------------------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        static ImVec2 global_mouse_pos; // 전역 마우스 좌표 저장용 변수 추가
        static Display* display = XOpenDisplay(NULL);

        if (display) {
            Window root = DefaultRootWindow(display);
            Window root_return, child_return;
            int root_x, root_y, win_x, win_y;
            unsigned int mask_return;

            // X11 API를 통해 OS 전역 마우스 좌표를 쿼리합니다.
            if (XQueryPointer(display, root, &root_return, &child_return,
                          &root_x, &root_y, &win_x, &win_y, &mask_return)) {

                // 1. 현재 창의 모니터 상 위치를 가져옵니다.[cite: 1]
                Vector2 window_pos = GetWindowPosition();

                // 2. 전역 마우스 좌표에서 창의 위치를 빼서 지역 좌표로 변환하여 ImGui에 주입합니다.
                ImGui::GetIO().AddMousePosEvent((float)root_x - window_pos.x, (float)root_y - window_pos.y);
                global_mouse_pos = ImVec2((float)root_x, (float)root_y);
                          }
        }

        ImGui::NewFrame();

        if (ImGui::GetIO().WantCaptureMouse) {
            ClearWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
            // SetWindowState(FLAG_WINDOW_TOPMOST);
        } else {
            SetWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
            // ClearWindowState(FLAG_WINDOW_TOPMOST);
        }

        static int last_theme = -1;
        if (last_theme != ImGuiExt::theme_id) {
            if (ImGuiExt::theme_id == 0) ImGui::style_white();
            else                         ImGui::style_dark();
            last_theme = ImGuiExt::theme_id;
        }

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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetColorU32(ImGuiCol_ChildBg));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);

        ImGui::Begin("CustomTitleBar", nullptr, titleFlags);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            is_dragging_title_bar = true;

            // ImGui의 상대 좌표가 아닌, 방금 캡처한 OS 절대 화면 마우스 좌표를 사용합니다.
            Vector2 winPos = GetWindowPosition();

            // 창의 좌상단 기준 클릭한 오프셋을 계산하여 저장합니다.
            drag_offset.x = global_mouse_pos.x - winPos.x;
            drag_offset.y = global_mouse_pos.y - winPos.y;
        }

        if (is_dragging_title_bar) {
            if (ImGui::IsMouseDown(0)) {
                // 드래그 중에도 절대 화면 마우스 좌표를 기준으로 창 위치를 업데이트합니다.
                ::SetWindowPosition((int)(global_mouse_pos.x - drag_offset.x),
                                    (int)(global_mouse_pos.y - drag_offset.y));
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


        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(125, 125, 125, 20));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(125, 125, 125, 30));

        if (ImGui::ButtonX(ICON_MD_SETTINGS, ImVec2(settingsBtnWidth, TITLEBAR_HEIGHT), ImGuiButtonFlags_None)) {
            ImGui::OpenPopup("SettingsPopup");
        }

        ImGui::PopStyleColor(3);

        // 설정 팝업 정의 (팝업 위치도 동일하게 조정)
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - (closeBtnWidth + maxBtnWidth + settingsBtnWidth) * 2, viewport->Pos.y + TITLEBAR_HEIGHT));
        if (ImGui::BeginPopup("SettingsPopup")) {

            ImGui::MenuItem("3D 뷰포트", "", &ImGuiExt::show_3d_viewport);
            ImGui::MenuItem("로거", "", &ImGuiExt::show_log_window);
            ImGui::MenuItem("스타일 에디터", "", &ImGuiExt::show_style_edit);

            if (ImGui::BeginMenu("테마"))
            {
                ImGui::ThemeSelector(&ImGuiExt::theme_id);
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        // --- 최대화/이전 크기로 복원 버튼 ---
        ImGui::SameLine(viewport->Size.x - closeBtnWidth - maxBtnWidth);
        ImGui::SetCursorPosY(0.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(125, 125, 125, 20));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(125, 125, 125, 30));

        // 창 상태에 따라 아이콘 텍스트 분기 처리
        const char* maxIcon = IsWindowMaximized() ? ICON_MD_FULLSCREEN_EXIT : ICON_MD_FULLSCREEN;

        if (ImGui::ButtonX(maxIcon, ImVec2(maxBtnWidth, ::TITLEBAR_HEIGHT), ImGuiButtonFlags_None)) {
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
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(181, 65, 66, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(145, 51, 48, 255));


        if (ImGui::ButtonX(ICON_MD_CLOSE, ImVec2(closeBtnWidth, TITLEBAR_HEIGHT), ImGuiButtonFlags_None)) {
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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::Begin("MainRootDockSpaceWindow", nullptr, dockFlags);
        ImGui::PopStyleColor(1);
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
                UpdateCamera(&ImGuiExt::camera, CAMERA_FREE);
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
                BeginMode3D(ImGuiExt::camera);

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
                    draw_axes(0.5, 0.01);

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
            loggr.draw(" " ICON_MD_SUBJECT " 로그 ", &ImGuiExt::show_log_window);


        // ---------------------------------------------------------------
        // Color Editer
        // ---------------------------------------------------------------
        if (ImGuiExt::show_style_edit) {
            ImGui::Begin(" " ICON_MD_STYLE " Style Editor ");
            ImGui::ShowStyleEditor(&ImGui::GetStyle());
            ImGui::End();
        }


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



        return true;
    }


    void style_white()
    {
        if (ImGuiExt::theme_id != 0) return;

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
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 1.00);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00);
        // [Tab]
        colors[ImGuiCol_Tab]                   = color_bg;
        colors[ImGuiCol_TabHovered]            = color_primary_hover;
        colors[ImGuiCol_TabSelected]           = color_tab_active;
        colors[ImGuiCol_TabUnfocused]          = color_bg;
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.24, 0.34, 0.44, 1.00);
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


    void style_dark()
    {
        if (ImGuiExt::theme_id != 1) return;
        auto& colors = ImGui::GetStyle().Colors;

        ImVec4 color_bg                 = ImColor(7, 7, 9);
        ImVec4 color_surf               = ImColor(12, 12, 14);
        ImVec4 color_surf_variant       = ImColor(11, 11, 12);


        ImVec4 color_primary            = ImColor(93, 105, 240);
        ImVec4 color_primary_hover      = ImColor(73, 85, 185);
        ImVec4 color_primary_active     = ImColor(63, 74, 162);


        ImVec4 color_red                = ImColor(181, 65, 60);
        ImVec4 color_green              = ImColor(87, 242, 135);

        ImVec4 color_text               = ImColor(228, 228, 230);
        ImVec4 color_text_disabled      = ImColor(128, 133, 138);
        ImVec4 color_transparent        = ImColor(0, 0, 0, 0);


        ImVec4 theme_tab_focused        = ImColor(18, 18, 20);
        ImVec4 theme_tab_active         = ImColor(36, 36, 39);

        ImVec4 color_border             = ImColor(44, 44, 47);

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
        colors[ImGuiCol_Header]                = theme_tab_focused;
        colors[ImGuiCol_HeaderHovered]         = color_primary_hover;
        colors[ImGuiCol_HeaderActive]          = color_primary_active;
        // [Separator]
        colors[ImGuiCol_Separator]             = ImVec4(0.28, 0.29, 0.30, 1.00);
        colors[ImGuiCol_SeparatorHovered]      = color_primary_hover;
        colors[ImGuiCol_SeparatorActive]       = color_primary_active;
        // [Resize Grip]
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 1.00);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00);
        // [Tab]
        colors[ImGuiCol_Tab]                   = color_bg;
        colors[ImGuiCol_TabHovered]            = color_primary_hover;
        colors[ImGuiCol_TabSelected]           = theme_tab_active;
        colors[ImGuiCol_TabUnfocused]          = color_bg;
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.24, 0.34, 0.44, 1.00);
        colors[ImGuiCol_TabSelectedOverline]   = color_transparent;
        colors[ImGuiCol_TabDimmed]             = color_bg;
        colors[ImGuiCol_TabDimmedSelected]     = theme_tab_active;
        // [Plot]
        colors[ImGuiCol_PlotLines]             = color_primary;
        colors[ImGuiCol_PlotLinesHovered]      = color_primary_hover;
        colors[ImGuiCol_PlotHistogram]         = color_primary;
        colors[ImGuiCol_PlotHistogramHovered]  = color_primary_hover;
        // [Table]
        colors[ImGuiCol_TableHeaderBg]         = ImColor(24, 24, 26);
        colors[ImGuiCol_TableBorderStrong]     = color_border;
        colors[ImGuiCol_TableBorderLight]      = ImColor(32, 32, 35);
        colors[ImGuiCol_TableRowBg]            = color_transparent;
        colors[ImGuiCol_TableRowBgAlt]         = ImColor(16, 16, 18);
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

    void help(const char *desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}


