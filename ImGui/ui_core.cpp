#include "ui.hpp"
#include "ui_font_icon.cpp"
#include "ui_font_level2.cpp"
#include "ui_font_d2coding.cpp"

#include <filesystem>
#include <fstream>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <cmath>
#include <csignal>
namespace fs = std::filesystem;

namespace
{
    bool window_pinned = false;
    int previous_window_focus = -1;
    Window previous_active_window = None;
    std::string config_path;
    float ui_content_scale = 1.0f;

    void update_content_scale(GLFWwindow* window)
    {
        const float scale = ImGui_ImplGlfw_GetContentScaleForWindow(window);
        if (!std::isfinite(scale) || scale <= 0.0f || std::fabs(scale - ui_content_scale) < 0.001f)
            return;

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale / ui_content_scale);
        style.FontScaleDpi *= scale / ui_content_scale;
        ui_content_scale = scale;
    }

    void update_window_stacking()
    {
        Display* display = glfwGetX11Display();
        Window window = glfwGetX11Window(glfwGetCurrentContext());
        const bool focused = glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_FOCUSED);
        const bool focus_changed = previous_window_focus != (int)focused;
        previous_window_focus = focused;
        static const Atom state_atom = XInternAtom(display, "_NET_WM_STATE", False);
        static const Atom above_atom = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
        static const Atom below_atom = XInternAtom(display, "_NET_WM_STATE_BELOW", False);
        static const Atom active_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
        Atom actual_type;
        int format;
        unsigned long count, remaining;
        unsigned char* data = nullptr;
        // Switching between two other apps (including across monitors) does
        // not send another FocusOut to us. Track desktop activation as well.
        Window active = None;
        if (XGetWindowProperty(display, DefaultRootWindow(display), active_atom,
                               0, 1, False, XA_WINDOW, &actual_type, &format,
                               &count, &remaining, &data) == Success &&
            actual_type == XA_WINDOW && format == 32 && count == 1)
            active = *reinterpret_cast<Window*>(data);
        if (data) XFree(data);
        data = nullptr;
        const bool activation_changed = active != None && active != previous_active_window;
        if (active != None) previous_active_window = active;
        const bool restack = focus_changed || activation_changed;
        const int result = XGetWindowProperty(display, window, state_atom, 0, 1024,
                                             False, XA_ATOM, &actual_type, &format,
                                             &count, &remaining, &data);
        bool above = false, below = false;
        if (result == Success && actual_type == XA_ATOM && format == 32) {
            const Atom* states = reinterpret_cast<const Atom*>(data);
            for (unsigned long i = 0; i < count; ++i) {
                above |= states[i] == above_atom;
                below |= states[i] == below_atom;
            }
        }
        if (data) XFree(data);
        if (result != Success) return;

        // Read the WM's actual state rather than raylib's cached TOPMOST flag.
        // Focus changes and WM actions can invalidate a previously sent request.
        auto set_state = [&](Atom atom, bool enabled) {
            XEvent event = {};
            event.xclient.type = ClientMessage;
            event.xclient.display = display;
            event.xclient.window = window;
            event.xclient.message_type = state_atom;
            event.xclient.format = 32;
            event.xclient.data.l[0] = enabled ? 1 : 0; // ADD / REMOVE
            event.xclient.data.l[1] = atom;
            event.xclient.data.l[3] = 1; // Request from an application.
            XSendEvent(display, DefaultRootWindow(display), False,
                       SubstructureRedirectMask | SubstructureNotifyMask, &event);
        };
        const bool want_above = window_pinned;
        const bool want_below = !window_pinned && !focused;
        // Remove the opposite layer before requesting a new one.
        if (above && !want_above) set_state(above_atom, false);
        if (below && !want_below) set_state(below_atom, false);
        if (above && want_above && restack) set_state(above_atom, false);
        if (want_above && (!above || restack)) set_state(above_atom, true);
        if (!below && want_below) set_state(below_atom, true);
        if ((window_pinned && restack) || above != want_above || below != want_below) XFlush(display);
    }

    // Let X11 hit-test the visible UI in the normal desktop stacking order.
    // Toggling passthrough from WantCaptureMouse cannot account for other apps
    // covering our window, and also makes the first click depend on frame timing.
    void update_input_region()
    {
        Display* display = glfwGetX11Display();
        static const bool has_input_shape = [display]() {
            int event_base, error_base, major, minor;
            return XShapeQueryExtension(display, &event_base, &error_base) &&
                   XShapeQueryVersion(display, &major, &minor) &&
                   (major > 1 || (major == 1 && minor >= 1));
        }();
        if (!has_input_shape) return;

        Window window = glfwGetX11Window(glfwGetCurrentContext());
        ImGuiContext& g = *ImGui::GetCurrentContext();
        // Popups need outside clicks for dismissal; X11 still respects other
        // applications above this native window.
        if (g.OpenPopupStack.Size > 0) {
            XShapeCombineMask(display, window, ShapeInput, 0, 0, None, ShapeSet);
        } else {
            Region region = XCreateRegion();
            for (ImGuiWindow* ui_window : g.Windows) {
                if (!ui_window->Active || ui_window->Hidden ||
                    (ui_window->Flags & ImGuiWindowFlags_NoMouseInputs)) continue;

                ImRect bounds = ui_window->OuterRectClipped;
                ImVec2 padding = g.Style.TouchExtraPadding;
                if (!(ui_window->Flags & (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)))
                    padding = ImMax(padding, ImVec2(g.Style.WindowBorderHoverPadding,
                                                   g.Style.WindowBorderHoverPadding));
                bounds.Expand(padding);
                bounds.ClipWith(ImRect(ImVec2(0, 0), g.IO.DisplaySize));
                int x = (int)std::floor(bounds.Min.x);
                int y = (int)std::floor(bounds.Min.y);
                int width = (int)std::ceil(bounds.Max.x) - x;
                int height = (int)std::ceil(bounds.Max.y) - y;
                if (width <= 0 || height <= 0) continue;
                XRectangle rect = { (short)x, (short)y,
                                    (unsigned short)width, (unsigned short)height };
                XUnionRectWithRegion(&rect, region, region);
            }
            XShapeCombineRegion(display, window, ShapeInput, 0, 0, region, ShapeSet);
            XDestroyRegion(region);
        }
        XFlush(display);
    }

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
    bool is_resizing            = false;
    bool is_dragging_title_bar  = false;
    Vector2 drag_offset         = { 0.0, 0.0 };


    // ==================================================
    // 폰트 설정
    // ==================================================
    const float  FONT_SIZE   = 20;
    const float  ICON_SIZE   = 28;
    const ImVec2 ICON_OFFSET = ImVec2(0, 6);
}


static void sigint_handler(int signum)
{
    ImGui::ShouldClose(true);
}




namespace ImGui
{
    static void ShowMenu()
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoDocking |
                                 ImGuiWindowFlags_NoScrollWithMouse |
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

        // 타이틀 바 드래그 이동 처리 (버튼 4개 크기만큼 제외)
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##title_drag", ImVec2(ImMax(1.0f, windowSize.x - (button_width * 4)), titlebar_height));
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


        // 핀 고정 버튼 (우측에서 4번째)
        ImGui::SetCursorPos(ImVec2(windowSize.x - (button_width * 4), 0));
        // Read the accent colors before overriding the button palette.
        const ImVec4 pin_bg = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 pin_hovered = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
        const ImVec4 pin_active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Button, window_pinned ? pin_bg : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, window_pinned ? pin_hovered : ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, window_pinned ? pin_active : ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        ImGui::PushStyleColor(ImGuiCol_Text, window_pinned ? ImVec4(1, 1, 1, 1) : ImGui::GetStyleColorVec4(ImGuiCol_Text));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        if (ImGui::IconButton(ICON_MD_KEEP "##window_pin", ImVec2(button_width, titlebar_height), false)) {
            window_pinned = !window_pinned;
            update_window_stacking();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", window_pinned ? "고정 해제 " : "항상 위에 고정 ");
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        // 1. 설정 버튼 (우측에서 3번째)
        ImGui::SetCursorPos(ImVec2(windowSize.x - (button_width * 3), 0));

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        if (ImGui::IconButton(ICON_MD_SETTINGS "##window_setting", ImVec2(button_width, titlebar_height), false))
        {
            ImGui::OpenPopup("SettingsPopup");
        }
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopup("SettingsPopup")) {

            bool config_changed = ImGui::MenuItem("3D Viewport", "", &ImGui::show_3d_viewport);
            config_changed |= ImGui::MenuItem("Log", "", &ImGui::show_log_window);
            ImGui::MenuItem("Style Editor", "", &ImGui::show_style_edit);
            config_changed |= ImGui::MenuItem("System HUD", "", &ImGui::show_system_hud);

            if (ImGui::BeginMenu("Theme"))
            {
                if (ImGui::ThemeSelector(&ImGui::theme_id)) {
                    ImGui::flag_change_thema = true;
                    config_changed = true;
                }
                ImGui::EndMenu();
            }

            if (config_changed && ImGui::GetIO().IniFilename != nullptr)
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            ImGui::EndPopup();
        }


        // 2. 최소화 버튼 (우측에서 2번째)
        ImGui::SetCursorPos(ImVec2(windowSize.x - (button_width * 2), 0));

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        // 머터리얼 아이콘에서 일반적으로 최소화를 나타내는 마이너스 기호(REMOVE) 사용
        if (ImGui::IconButton(ICON_MD_REMOVE "##window_minimaze", ImVec2(button_width, titlebar_height), false))
        {
            MinimizeWindow();
        }

        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(3);


        // 3. 닫기 버튼 (가장 우측)
        ImGui::SetCursorPos(ImVec2(windowSize.x - button_width, 0));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(181, 65, 66, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(145, 51, 48, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        if (ImGui::IconButton(ICON_MD_CLOSE, ImVec2(button_width, titlebar_height), false))
        {
            ImGui::ShouldClose(true);
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
    }

    void Init(const char* title, int width, int height)
    {
        ImGui::InitLogger();
        std::signal(SIGINT, sigint_handler);

        WINDOW_TITLE = title;

        // ---------------------------------------------------------------
        // ImGui & raylib 초기화
        // ---------------------------------------------------------------
        SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN);
        InitWindow(width, height, title);
        // Set the initial maximized state before mapping the X11 window.
        MaximizeWindow();
        window_pinned = false;
        previous_window_focus = -1;
        previous_active_window = None;
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


        ImFontConfig icon_config;
        icon_config.MergeMode = true;
        icon_config.GlyphOffset = ICON_OFFSET;
        icon_config.GlyphMinAdvanceX = FONT_SIZE;
        static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_16_MD, 0 };

        // 기본 폰트 로딩
        io.Fonts->AddFontFromMemoryCompressedTTF(
            level2_compressed_data, level2_compressed_size,
            FONT_SIZE,
            NULL,
            io.Fonts->GetGlyphRangesKorean()
        );

        // 아이콘 폰트 로딩
        io.Fonts->AddFontFromMemoryCompressedTTF(
            MaterialSymbolsRounded_compressed_data,
            MaterialSymbolsRounded_compressed_size,
            ICON_SIZE,
            &icon_config,
            icon_ranges
        );

        // 로거 전용 폰트 로딩
        ImGui::LOGGER_FONT = io.Fonts->AddFontFromMemoryCompressedTTF(
            font2_compressed_data,
            font2_compressed_size,
            FONT_SIZE,
            NULL,
            io.Fonts->GetGlyphRangesKorean()
        );


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
        ui_content_scale = 1.0f;
        update_content_scale(window);

        view_texture = LoadRenderTexture((int)WINDOW_WIDTH, (int)WINDOW_HEIGHT);
        ClearWindowState(FLAG_WINDOW_HIDDEN);
    }

    void Destroy()
    {
        destroy_textures();
        UnloadRenderTexture(::view_texture);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImPlot3D::DestroyContext();
        ImGui::DestroyContext();
        CloseWindow();
        ImGui::ShutdownLogger();
    }

    bool Context(std::function<void()> func)
    {
        if (ShouldClose()) {
            return false;
        }


        // -----------------------
        // 1. ImGui 프레임 시작
        // -----------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        update_content_scale(glfwGetCurrentContext());
        update_window_stacking();

        // -----------------------
        // 전역 좌표는 HUD용으로만 사용한다. ImGui 입력은 GLFW가 전달한다.
        // -----------------------
        Display* display = glfwGetX11Display();

        if (display) {
            Window root = DefaultRootWindow(display);
            Window root_return, child_return;
            int root_x, root_y, win_x, win_y;
            unsigned int mask_return;

            // X11 API를 통해 OS 전역 마우스 좌표를 쿼리
            if (XQueryPointer(display, root, &root_return, &child_return, &root_x, &root_y, &win_x, &win_y, &mask_return)) {

                ImGui::GLOBAL_MOUSE_POS = ImVec2((float)root_x, (float)root_y);
            }
        }

        ImGui::NewFrame();


        // -----------------------
        // 테마 변경
        // -----------------------
        if (ImGui::flag_change_thema) {
            ImGui::flag_change_thema = false;

            if (ImGui::theme_id == 0) ImGui::theme_white();
            else                      ImGui::theme_dark();
        }

        // -----------------------
        // 메뉴 렌더링
        // -----------------------
        ImGui::ShowMenu();

        // -----------------------
        // ImGui log 렌더링
        // -----------------------
        if (ImGui::show_log_window)
        ImGui::ShowLogger(" " ICON_MD_SUBJECT " Log ");

        // -----------------------
        // Style Editer
        // -----------------------
        if (ImGui::show_style_edit) {
            ImGui::ShowStyleEditor(&ImGui::GetStyle());
        }

        // -----------------------
        // 3. 렌더링된 텍스처를 담을 뷰포트 창 띄우기
        // -----------------------
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

        // -----------------------
        // ImGui notiy 렌더링
        // -----------------------
        ImGui::RenderNotifications();

        // -----------------------
        // System HUD
        // -----------------------
        ImGui::draw_system_hud(ImGui::show_system_hud);

        // -----------------------
        // ImGui, Raylib 렌더링
        // -----------------------
        ImGui::Render();
        update_input_region();
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

    bool ShouldClose(bool force_close)
    {
        ImGui::should_close_app |= force_close;
        return WindowShouldClose() || ImGui::should_close_app;
    }

    void LoadConfig(const char* path)
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
        config_path = fs::absolute(p).string();
        ImGui::GetIO().IniFilename = config_path.c_str();


        // ini 콜백 등록
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName   = ""; // ini 파일에 기록될 섹션 이름
        ini_handler.TypeHash   = ImHashStr("");
        ini_handler.ReadOpenFn = read_open;
        ini_handler.ReadLineFn = read_line;
        ini_handler.WriteAllFn = write_all;
        if (ImGui::FindSettingsHandler(ini_handler.TypeName) == nullptr)
            ImGui::AddSettingsHandler(&ini_handler);

        ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
    }


    Vector2 get_viewport_mouse_pos()                 { return viewport_mouse_pos;  }
    bool    is_viewport_hovered()                    { return ::is_viewport_hovered; }
}
