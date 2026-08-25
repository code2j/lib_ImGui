#include "ui.hpp"
#include "ui_font_icon.cpp"
#include "ui_font_level2.cpp"
#include "ui_font_d2coding.cpp"

#include <filesystem>
#include <fstream>
#include <X11/Xlib.h>
#include <csignal>
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
    const float TITLEBAR_HEIGHT = 0.0;
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


static void sigint_handler(int signum)
{
    ImGui::should_close(true);
}


namespace ImGui
{


    void init(const char* title, int width, int height)
    {
        std::signal(SIGINT, sigint_handler);

        WINDOW_TITLE = title;

        // ---------------------------------------------------------------
        // ImGui & raylib 초기화
        // ---------------------------------------------------------------
        SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT);
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


        // ---------------------------------------------------------------
        // 테마 색상 변수 정의
        // ---------------------------------------------------------------
        ImGui::style();
        if (ImGui::theme_id == 0) ImGui::theme_white();
        else                      ImGui::theme_dark();


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
        destroy_textures();
        UnloadRenderTexture(::view_texture);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImPlot3D::DestroyContext();
        ImGui::DestroyContext();
        CloseWindow();
    }

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


        // 패스스루 마우스 이벤트
        static Display* display = XOpenDisplay(NULL);

        if (display) {
            Window root = DefaultRootWindow(display);
            Window root_return, child_return;
            int root_x, root_y, win_x, win_y;
            unsigned int mask_return;

            // X11 API를 통해 OS 전역 마우스 좌표를 쿼리
            if (XQueryPointer(display, root, &root_return, &child_return, &root_x, &root_y, &win_x, &win_y, &mask_return)) {

                Vector2 window_pos = GetWindowPosition();

                ImGui::GetIO().AddMousePosEvent((float)root_x - window_pos.x, (float)root_y - window_pos.y);
                ImGui::GLOBAL_MOUSE_POS = ImVec2((float)root_x, (float)root_y);
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


        // ---------------------------------------------------------------
        // Loading Screen
        // ---------------------------------------------------------------
        static int              lazy_cnt       = 0;
        static bool             lazy_flag1      = false;
        static bool             lazy_flag2      = false;

        static ImGui::Texture   loading_img;

        lazy_cnt++;
        if (lazy_cnt > 100)
            lazy_flag1 = true;



        if (!lazy_flag1) {
            // [레이지 스타트 카운트 증가중]

            // 이미지 중심이 화면 중심으로 계산
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos(
                ImVec2(display_size.x * 0.5f, display_size.y * 0.5f),
                ImGuiCond_Always,
                ImVec2(0.5f, 0.5f)
            );

            ImGui::SetNextWindowSize(ImVec2(300, 300));

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_NoBackground |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

            ImGui::Begin("##LoadingScreen", nullptr, flags);
            ImVec2 avail = ImGui::GetContentRegionAvail();

            if (auto tex = loading_img.lock()) {
                // [이미지 로딩됨]
                float img_aspect = (float) tex->width / (float) tex->height;
                float avail_aspect = avail.x / avail.y;

                ImVec2 size;
                if (avail_aspect > img_aspect) {
                    size.y = avail.y;
                    size.x = size.y * img_aspect;
                } else {
                    size.x = avail.x;
                    size.y = size.x / img_aspect;
                }

                float cursor_x = (avail.x - size.x) * 0.5f;
                float cursor_y = (avail.y - size.y) * 0.5f;
                ImGui::SetCursorPos(ImVec2(cursor_x, cursor_y));


                // 로딩 이미지 표시
                ImGui::Image(tex->id, size);


                // 아이콘 폰트 추가
                ImFontConfig config;
                config.MergeMode        = true;
                config.GlyphOffset      = ICON_OFFSET;
                config.GlyphMinAdvanceX = FONT_SIZE;
                static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };
                ImGuiIO& io = ImGui::GetIO();

                if (lazy_cnt == 1) {

                }
                else if (lazy_cnt == 2) {
                    // 아이콘 폰트
                    io.Fonts->AddFontFromMemoryCompressedTTF(
                        MaterialSymbolsRounded_compressed_data,
                        MaterialSymbolsRounded_compressed_size,
                        ICON_SIZE,
                        &config,
                        icon_ranges
                    );
                }
                else if (lazy_cnt == 3) {
                    // 로거 전용 폰트
                    ImGui::D2Cording = io.Fonts->AddFontFromMemoryCompressedTTF(
                        font2_compressed_data,
                        font2_compressed_size,
                        FONT_SIZE,
                        NULL,
                        io.Fonts->GetGlyphRangesKorean()
                    );
                }
            }
            else {
                // [이미지가 로딩 안됨]
                loading_img = ImGui::load_texture(IMGUI_ROOT "/image.png");
            }
            ImGui::End();
        }
        else {
            // [레이지 스타트 카운팅 넘음]
            ImGui::flag_load_complete = true;
        }



        // ---------------------------------------------------------------
        // 테마 변경
        // ---------------------------------------------------------------
        if (ImGui::flag_change_thema) {
            ImGui::flag_change_thema = false;

            if (ImGui::theme_id == 0) ImGui::theme_white();
            else                      ImGui::theme_dark();
        }
        // ---------------------------------------------------------------
        // 최초 로딩 완료
        // ---------------------------------------------------------------
        if (ImGui::flag_load_complete) {
            ImGui::flag_load_complete = false;
            MaximizeWindow();
            lazy_flag2 = true;
        }
        // ---------------------------------------------------------------
        // 메뉴 on/off 키 입력
        // ---------------------------------------------------------------
        if ( (IsKeyDown(KEY_LEFT_ALT) ||IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_P)) {
            ImGui::show_main_menu = !ImGui::show_main_menu;
        }




        if (lazy_flag2)
        {
            // ---------------------------------------------------------------
            // 2. 메인 메뉴
            // ---------------------------------------------------------------
            if (ImGui::show_main_menu)
            {
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoReserveScrollbar |
                                         ImGuiWindowFlags_NoDocking |
                                         ImGuiWindowFlags_NoScrollbar;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));
                ImGui::Begin("window", nullptr, flags);
                ImGui::PopStyleVar();

                // ---------------------
                // 커스텀 타이틀바
                // ---------------------
                const float titlebar_height = 34.0f;
                const float button_width = 45.0f;
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 windowSize = ImGui::GetWindowSize();

                //  타이틀 바 배경 그리기
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                float windowRounding = ImGui::GetStyle().WindowRounding;

                drawList->AddRectFilled(
                    windowPos,
                    ImVec2(windowPos.x + windowSize.x, windowPos.y + titlebar_height),
                    ImGui::GetColorU32(ImGuiCol_WindowBg),
                    windowRounding,
                    ImDrawFlags_RoundCornersTop
                );

                // 타이틀 바 드래그 이동 처리 (버튼 2개 크기만큼 제외)
                ImGui::SetCursorPos(ImVec2(0, 0));
                ImGui::InvisibleButton("##title_drag", ImVec2(windowSize.x - (button_width * 2), titlebar_height));
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    ImGui::SetWindowPos(ImVec2(windowPos.x + delta.x, windowPos.y + delta.y));
                }

                // 타이틀 텍스트 출력
                const char* titleText = WINDOW_TITLE.c_str();
                ImVec2 textSize = ImGui::CalcTextSize(titleText);

                float textPosX = (windowSize.x - textSize.x) * 0.5f;
                float textPosY = (titlebar_height - textSize.y) * 0.3f;

                ImGui::SetCursorPos(ImVec2(textPosX, textPosY));
                ImGui::Text("%s", titleText);


                //  설정 버튼
                ImGui::SetCursorPos(ImVec2(windowSize.x - (button_width * 2), 0));

                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(125, 125, 125, 20));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(125, 125, 125, 30));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

                if (ImGui::ButtonX(ICON_MD_SETTINGS, ImVec2(button_width, titlebar_height), false)) {
                    ImGui::OpenPopup("SettingsPopup");
                }

                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(3);

                if (ImGui::BeginPopup("SettingsPopup")) {

                    ImGui::MenuItem("3D Viewport", "", &ImGui::show_3d_viewport);
                    ImGui::MenuItem("Log", "", &ImGui::show_log_window);
                    ImGui::MenuItem("Style Editor", "", &ImGui::show_style_edit);
                    ImGui::MenuItem("System HUD", "", &ImGui::show_system_hud);

                    if (ImGui::BeginMenu("Theme"))
                    {
                        if (ImGui::ThemeSelector(&ImGui::theme_id)) {
                            ImGui::flag_change_thema = true;
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndPopup();
                }


                // 닫기 버튼
                ImGui::SetCursorPos(ImVec2(windowSize.x - button_width, 0));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(181, 65, 66, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(145, 51, 48, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

                if (ImGui::ButtonX(ICON_MD_CLOSE, ImVec2(button_width, titlebar_height), false))
                {
                    ImGui::should_close(true);
                }

                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(3);
                // --- 타이틀 바 끝 ---

                // 영역을 DockSpace로 설정
                ImGui::SetCursorPosY(titlebar_height);
                ImGuiID dockspace_id = ImGui::GetID("MyInternalDockSpace");
                ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, IM_COL32(10, 10, 10, 0));
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
                ImGui::PopStyleColor(1);

                ImGui::End();



                // ---------------------------------------------------------------
                // ImGui log 렌더링
                // ---------------------------------------------------------------
                if (ImGui::show_log_window)
                    loggr.draw(" " ICON_MD_SUBJECT " Log ", &ImGui::show_log_window);

                // ---------------------------------------------------------------
                // Style Editer
                // ---------------------------------------------------------------
                if (ImGui::show_style_edit) {
                    ImGui::Begin(" " ICON_MD_STYLE " Style Editor ");
                    ImGui::ShowStyleEditor(&ImGui::GetStyle());
                    ImGui::End();
                }
            }


            // ---------------------------------------------------------------
            // 3. 렌더링된 텍스처를 담을 뷰포트 창 띄우기
            // ---------------------------------------------------------------
            if (ImGui::show_main_menu)
            if (ImGui::show_3d_viewport)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

                ImGui::Begin(" " ICON_MD_DEPLOYED_CODE " 3D Viewport ");

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
                // 4. 사용자 콘텐츠 렌더링 (Raylib 텍스처 + 사용자 ImGui UI)
                // ---------------------------------------------------------------
                BeginTextureMode(view_texture);
                ClearBackground(BLANK);

                // 커서가 숨겨져 있을 때 (카메라 조작 중)
                if (IsCursorHidden()) {
                    // [뷰포트 호버 여부와 상관없이 우클릭으로 무조건 커서 복구]
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                        EnableCursor();
                    }
                    // 카메라 업데이트 수행
                    UpdateCamera(&ImGui::camera, CAMERA_FREE);
                }
                // 커서가 보일 때 (일반 UI 조작 중)
                else {
                    // [뷰포트 위에 있을 때만 우클릭으로 커서 숨기기 (카메라 조작 시작)]
                    if (ImGui::is_viewport_hovered()) {
                        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                            DisableCursor();
                        }
                    }
                }


                ImGui::PopStyleVar();

                // 사용자 람다 콜백 실행
                if (func) {
                    BeginMode3D(ImGui::camera);
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
            // ImGui notiy 렌더링
            // ---------------------------------------------------------------
            ImGui::RenderNotifications();


            // ---------------------------------------------------------------
            // System HUD
            // ---------------------------------------------------------------
            if (ImGui::show_system_hud)
                ImGui::draw_system_hud();


        }




        // ---------------------------------------------------------------
        // ImGui, Raylib 렌더링
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

    bool should_close(bool force_close)
    {
        ImGui::should_close_app |= force_close;
        return WindowShouldClose() || ImGui::should_close_app;
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

    void    show_menu(bool b)                        { show_main_menu = b; }
    Vector2 get_viewport_mouse_pos()                 { return viewport_mouse_pos;  }
    bool    is_viewport_hovered()                    { return ::is_viewport_hovered; }



}


