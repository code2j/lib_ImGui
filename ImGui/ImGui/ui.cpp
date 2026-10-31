#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui.hpp"
#include "ui_font_icon.cpp"
#include "ui_font_level2.cpp"
#include "ui_font_d2coding.cpp"
#include "ui_3d.h"

#include <filesystem>
#include <fstream>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <cmath>
#include <csignal>
#include <vector>
namespace fs = std::filesystem;

namespace
{
    std::string config_path;


    // ==================================================
    // 윈도우 하상 위에 고정 상태 변수
    // ==================================================
    bool window_pinned = false;                 // 항상 위에 고정 여부
    int previous_window_focus = -1;             // 이전 윈도우 포커스 여부
    Window previous_active_window = None;       // 활성화된 윈도우 객체

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
    // DPI
    // ==================================================
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


    // ==================================================
    // 사이드바
    // ==================================================
    struct SidebarTab {
        std::string label;
        std::function<void()> draw, sidebar_content;
        float expanded = 0.0f, content_height = 0.0f;
        bool open = false;
    };
    std::vector<SidebarTab> SIDETAP_BAR;


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







namespace ImGui
{
    void Init(const char* title, int width, int height);
    void Destroy();
    bool Context(std::function<void()> func);
    void LoadConfig(const char* path);
    bool ShouldClose(bool force_close);
    void AddTab(const char* label, std::function<void()> draw, std::function<void()> sidebar_content);

    static void ShowMenu();
    static bool FakeLoading();


    static void ChangeStyle();


    // 구현
    void Init(const char* title, int width, int height)
    {
        std::signal(SIGINT, [](int){ImGui::ShouldClose(true);});

        ImGui::InitLogger();

        WINDOW_TITLE = title;

        // ---------------------------------------------------------------
        // ImGui & raylib 초기화
        // ---------------------------------------------------------------
        SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN);
        InitWindow(width, height, title);
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
        io.ConfigWindowsMoveFromTitleBarOnly = true;          // 타이틀바만 드래깅 허용


        // ---------------------------------------------------------------
        // 폰트 로딩
        // ---------------------------------------------------------------
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
            d2cording_compressed_data,
            d2cording_compressed_size,
            FONT_SIZE,
            NULL,
            io.Fonts->GetGlyphRangesKorean()
        );


        // ---------------------------------------------------------------
        // 테마 색상 변수 정의
        // ---------------------------------------------------------------
        ImGui::Style();
        if (ImGui::theme_id == 0) ImGui::ThemeWhite();
        else                      ImGui::ThemeDark();


        // ---------------------------------------------------------------
        // 윈도우 및 뷰포트 텍스쳐 생성
        // ---------------------------------------------------------------
        GLFWwindow* window = glfwGetCurrentContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
        ui_content_scale = 1.0f;
        update_content_scale(window);

        ClearWindowState(FLAG_WINDOW_HIDDEN);
    }

    void Destroy()
    {
        SIDETAP_BAR.clear();
        destroy_textures();
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


        // ImGui 프레임 시작
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        update_content_scale(glfwGetCurrentContext());
        update_window_stacking();
        ImGui::NewFrame();

        ChangeStyle();


        if (!FakeLoading()) {
            ImGui::ShowMenu();
            ImGui::ShowLogger(" " ICON_MD_SUBJECT " Log ", ImGui::show_log_window);
            ImGui::ShowNotifications();
            ImGui::ShowSystemHud(ImGui::show_system_hud);
            ImGui::Show3D(" " ICON_MD_DEPLOYED_CODE " 3D Viewport ",  ImGui::show_3d_viewport, func);
            if (ImGui::show_style_edit) {
                ImGui::ShowStyleEditor(&ImGui::GetStyle());
            }

            if (func) func();
        }


        // ImGui, Raylib 렌더링
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

    bool ShouldClose(bool force_close)
    {
        ImGui::should_close_app |= force_close;
        return WindowShouldClose() || ImGui::should_close_app;
    }

    void AddTab(const char* label, std::function<void()> draw, std::function<void()> sidebar_content)
    {
        if (!label || !*label) return;
        for (auto& tab : SIDETAP_BAR) {
            if (tab.label == label) {
                tab.draw = std::move(draw);
                tab.sidebar_content = std::move(sidebar_content);
                return;
            }
        }
        SIDETAP_BAR.push_back({label, std::move(draw), std::move(sidebar_content)});
    }

    static void ShowMenu()
    {
        // 화면 배율에 맞춰 상단 헤더와 버튼의 크기 및 간격을 정한다.
        const float scale = ui_content_scale;
        const float header_height = 68.0f * scale;
        const float button_size = 36.0f * scale;
        const float gap = 6.0f * scale;
        const float inset = 16.0f * scale;
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoScrollWithMouse |
                                       ImGuiWindowFlags_NoScrollbar;

        // 기본 윈도우
        ImGui::SetNextWindowSizeConstraints(ImVec2(380, 200) * scale, ImVec2(FLT_MAX, FLT_MAX));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * scale);
        const bool visible = ImGui::Begin("window", nullptr, flags);
        ImGui::PopStyleVar(2);
        if (!visible) {
            ImGui::End();
            return;
        }

        // 창 상단에 둥근 헤더 배경과 본문을 구분하는 선을 그린다.
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        const float actions_x = size.x - inset - button_size * 4 - gap * 3;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, pos + ImVec2(size.x, header_height),
                            ImGui::GetColorU32(ImGuiCol_ChildBg), 12.0f * scale,
                            ImDrawFlags_RoundCornersTop);
        draw->AddLine(pos + ImVec2(0, header_height), pos + ImVec2(size.x, header_height),
                      ImGui::GetColorU32(ImGuiCol_Border));

        // 해더 영역 드래깅
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##title_drag", ImVec2(actions_x - gap, header_height));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            ImGui::SetWindowPos(pos + ImGui::GetIO().MouseDelta);

        // 헤더 왼쪽에 위젯 아이콘 배지와 창 제목, WORKSPACE 부제를 그린다.
        const ImVec2 badge = pos + ImVec2(inset, 16.0f * scale);
        draw->AddRectFilled(badge, badge + ImVec2(button_size, button_size), ImGui::GetColorU32(ImGuiCol_Button), 10.0f * scale);
        const ImVec2 icon_size = ImGui::CalcTextSize(ICON_MD_WIDGETS);
        draw->AddText(badge + (ImVec2(button_size, button_size) - icon_size) * 0.5f, IM_COL32(255, 255, 255, 255), ICON_MD_WIDGETS);
        const float text_x = inset + button_size + 12.0f * scale;
        draw->PushClipRect(pos + ImVec2(text_x, 0), pos + ImVec2(actions_x - gap, header_height), true);
        draw->AddText(pos + ImVec2(text_x, 10.0f * scale), ImGui::GetColorU32(ImGuiCol_Text), WINDOW_TITLE.c_str());
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.72f, pos + ImVec2(text_x, 39.0f * scale), ImGui::GetColorU32(ImGuiCol_TextDisabled), "WORKSPACE");
        draw->PopClipRect();

        // 공통 버튼 스타일
        auto action = [&](int index, const char* icon, const char* tooltip, bool selected = false, bool destructive = false)
        {
            ImGui::SetCursorPos(ImVec2(actions_x + index * (button_size + gap), 16.0f * scale));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f * scale);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImGui::GetStyleColorVec4(ImGuiCol_Button) : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, destructive ? ImVec4(ImColor(181, 65, 66)) : ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, destructive ? ImVec4(ImColor(145, 51, 48)) : ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
            ImGui::PushStyleColor(ImGuiCol_Text, selected ? ImVec4(1, 1, 1, 1) : ImGui::GetStyleColorVec4(ImGuiCol_Text));
            const bool pressed = ImGui::IconButton(icon, ImVec2(button_size, button_size), ImGuiButtonFlags_None);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("%s", tooltip);
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(3);
            return pressed;
        };

        // 고정 핀 버튼
        if (action(0, ICON_MD_KEEP "##window_pin", window_pinned ? "고정 해제" : "항상 위에 고정", window_pinned)) {
            window_pinned = !window_pinned;
            update_window_stacking();
        }

        // 설정 버튼
        if (action(1, ICON_MD_SETTINGS "##window_setting", "워크스페이스 설정", ImGui::IsPopupOpen("SettingsPopup")))
            ImGui::OpenPopup("SettingsPopup");

        // 최소화 버튼
        if (action(2, ICON_MD_REMOVE "##window_minimaze", "최소화"))
            MinimizeWindow();

        // 닫기 버튼
        if (action(3, ICON_MD_CLOSE "##window_close", "닫기", false, true))
            ImGui::ShouldClose(true);

        // 설정 팝업
        ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f * scale, 0), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10) * scale);
        if (ImGui::BeginPopup("SettingsPopup")) {
            ImGui::Spacing();

            ImGui::SeparatorText("Menu");
            bool config_changed = false;
            config_changed |= ImGui::MenuItem(ICON_MD_DEPLOYED_CODE "  3D Viewport", nullptr, &ImGui::show_3d_viewport);
            config_changed |= ImGui::MenuItem(ICON_MD_SUBJECT "  Log", nullptr, &ImGui::show_log_window);
            config_changed |= ImGui::MenuItem(ICON_MD_TUNE "  Style Editor", nullptr, &ImGui::show_style_edit);
            config_changed |= ImGui::MenuItem(ICON_MD_POSITION_TOP_RIGHT "  System HUD", nullptr, &ImGui::show_system_hud);
            ImGui::Spacing();

            // 테마 선택기
            ImGui::SeparatorText("Theme");
            if (ImGui::ThemeSelector(&ImGui::theme_id)) {
                ImGui::flag_change_thema = true;
                config_changed = true;
            }

            // 설정 저장
            if (config_changed && ImGui::GetIO().IniFilename != nullptr)
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(3);


        // 사이드 패널 영역 설정
        const float body_y = header_height + 8.0f * scale;
        const float sidebar_width = ImMin(220.0f * scale, size.x * 0.32f);
        ImGui::SetCursorPos(ImVec2(16.0f * scale, body_y + 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6) * scale);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
        ImGui::BeginChild("##sidebar", ImVec2(sidebar_width, -8.0f * scale), ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::Dummy(ImVec2(0, 0 * scale));

        // 사이드 패널 버튼 공통 스타일 설정
        static int selected_page = 0;
        const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 subtle_accent = ImVec4(accent.x, accent.y, accent.z, 0.28f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.08f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));

        const int page_count = (int)SIDETAP_BAR.size();
        if (selected_page >= page_count) selected_page = 0;

        // 사이드 패널 선택 애니메이션
        static float indicator_y = 0.0f;
        static bool indicator_initialized = false;
        float target_y = 0.0f, indicator_x = 0.0f;
        const float sidebar_origin_y = ImGui::GetWindowPos().y - ImGui::GetScrollY();
        for (int i = 0; i < page_count; ++i) {
            auto& tab = SIDETAP_BAR[i];
            const bool selected = selected_page == i;
            ImGui::PushStyleColor(ImGuiCol_Button, selected ? subtle_accent : ImVec4(0, 0, 0, 0));
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
            ImGui::PushID(i);
            const char* label = SIDETAP_BAR[i].label.c_str();
            // 선택된 탭을 다시 누르면 하위 내용을 접거나 펼친다.
            if (ImGui::Button(label, ImVec2(-FLT_MIN, 42.0f * scale))) {
                if (tab.sidebar_content)
                    tab.open = selected ? !tab.open : true;
                selected_page = i;
            }
            const ImVec2 button_min = ImGui::GetItemRectMin();
            if (selected_page == i) {
                target_y = button_min.y - sidebar_origin_y;
                indicator_x = button_min.x;
            }
            ImGui::PopID();
            if (selected) ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            // 하위 내용의 높이를 먼저 측정한 뒤 크기와 투명도를 함께 변경한다.
            if (tab.sidebar_content) {
                const bool measuring = tab.open && tab.content_height == 0.0f;
                if (!measuring) {
                    const float speed = ImGui::GetIO().DeltaTime * 7.0f;
                    tab.expanded = ImClamp(tab.expanded + (tab.open ? speed : -speed), 0.0f, 1.0f);
                }
                if (tab.expanded > 0.0f || measuring) {
                    ImGui::PushID(i);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, measuring ? 0.0f : tab.expanded);
                    ImGui::BeginChild("##sidebar_content", ImVec2(0, ImMax(0.01f, tab.content_height * tab.expanded)), 0,
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    const float start_y = ImGui::GetCursorScreenPos().y;
                    tab.sidebar_content();
                    tab.content_height = ImMax(1.0f, ImGui::GetCursorScreenPos().y - start_y);
                    ImGui::EndChild();
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
            }
        }

        // 선택 표시를 현재 탭 위치까지 부드럽게 이동시킨다.
        if (page_count > 0) {
            if (!indicator_initialized) { indicator_y = target_y; indicator_initialized = true; }
            indicator_y += (target_y - indicator_y) * (1.0f - std::exp(-12.0f * ImGui::GetIO().DeltaTime));
            const float width = 4.0f * scale;
            const float inset = 7.0f * scale;
            const float y = sidebar_origin_y + indicator_y;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(indicator_x, y + inset),
                ImVec2(indicator_x + width, y + 42.0f * scale - inset),
                ImGui::GetColorU32(accent), width * 0.5f);
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        ImGui::SetCursorPos(ImVec2(16.0f * scale + sidebar_width, body_y));
        // 등록된 탭마다 독립된 도킹 레이아웃을 사용한다.
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0, 0, 0, 0));
        std::vector<ImGuiID> all_dockspaces;
        all_dockspaces.reserve(SIDETAP_BAR.size());
        for (const auto& tab : SIDETAP_BAR) {
            const std::string id = "SidebarDockSpace:" + tab.label;
            all_dockspaces.push_back(ImGui::GetID(id.c_str()));
        }
        // 선택되지 않은 탭의 도킹 레이아웃도 유지한다.
        const ImVec2 dockspace_pos = ImGui::GetCursorScreenPos();
        for (int i = 0; i < page_count; ++i)
            if (i != selected_page) {
                ImGuiDockNode* node = ImGui::DockBuilderGetNode(all_dockspaces[i]);
                if (!node || !node->HostWindow) {
                    // 처음에는 호스트를 생성해 복원된 창이 비활성 탭에 유지되도록 한다.
                    ImGui::SetCursorScreenPos(dockspace_pos);
                    ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_None);
                }
                ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
            }
        // 선택된 탭의 도킹 영역과 내용을 표시한다.
        ImGui::SetCursorScreenPos(dockspace_pos);
        if (page_count > 0)
            ImGui::DockSpace(all_dockspaces[selected_page], ImVec2(0, 0), ImGuiDockNodeFlags_None);
        ImGui::PopStyleColor();
        ImGui::End();
        if (page_count > 0) {
            auto draw_tab = SIDETAP_BAR[selected_page].draw;
            if (draw_tab) draw_tab();
        }
    }

    static bool FakeLoading()
    {
        static double opening_start = ImGui::GetTime();
        const float opening_elapsed = (float)(ImGui::GetTime() - opening_start);
        auto ease_out = [](float t) { return 1.0f - (1.0f - t) * (1.0f - t); };
        float opening_progress;
        if (opening_elapsed < 0.65f) opening_progress = 0.38f * ease_out(opening_elapsed / 0.65f);
        else if (opening_elapsed < 0.85f) opening_progress = 0.38f;
        else if (opening_elapsed < 1.20f) opening_progress = 0.38f + 0.22f * ease_out((opening_elapsed - 0.85f) / 0.35f);
        // 중간 작업은 멈추지 않고 조금 더 천천히 진행한다.
        else if (opening_elapsed < 2.35f) opening_progress = 0.60f + 0.17f * ((opening_elapsed - 1.20f) / 1.15f);
        else if (opening_elapsed < 2.65f) opening_progress = 0.77f;
        else opening_progress = 0.77f + 0.23f * ease_out(ImClamp((opening_elapsed - 2.65f) / 0.95f, 0.0f, 1.0f));

        if (opening_elapsed >= 4.5f)
            return false;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float scale = ui_content_scale;
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(620.0f * scale, 390.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f * scale, 20.0f * scale));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
        ImGui::Begin("##opening", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("[ SYSTEM BOOT ]");
        ImGui::SameLine();
        ImGui::TextDisabled("// %s", WINDOW_TITLE.c_str());
        ImGui::Separator();
        static const char* lines[] = {
            "Initializing runtime environment...",
            "  > mount /dev/system --read-write",
            "  > scan modules: core, renderer, interface",
            "[ OK ] Memory allocation complete",
            "  > connect workspace://localhost",
            "  > load config --resolve-dependencies",
            "[ OK ] Graphics pipeline online",
            "  > 0x7F02: 48 8B 05 31 C0 90 FF",
            "  > compiling interface resources...",
            "[ OK ] System checks passed",
            "  > starting user session...",
            "  > decrypting workspace manifest...",
            "[ OK ] All services ready"
        };
        const int line_count = (int)(sizeof(lines) / sizeof(lines[0]));
        const int step = (int)(opening_progress * (line_count - 1));
        const float footer_height = 8.0f * scale + ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeight();
        const float lines_bottom = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - footer_height - 12.0f * scale;
        const int visible = ImMax(1, (int)((lines_bottom - ImGui::GetCursorPosY()) / ImGui::GetTextLineHeightWithSpacing()));
        const float lines_height = visible * ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetCursorPosY(ImMax(ImGui::GetCursorPosY(), lines_bottom - lines_height));
        for (int i = 0; i < visible; ++i) {
            int index = step - visible + 1 + i;
            if (index >= 0) ImGui::TextUnformatted(lines[index]);
            else ImGui::TextUnformatted(" ");
        }
        ImGui::SetCursorPosY(ImMax(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - footer_height));

        // 로딩바 그리기
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * scale);
        ImGui::ProgressBar(opening_progress, ImVec2(-1, 8.0f * scale), "");
        ImGui::PopStyleVar(2);
        ImGui::Text("BOOT SEQUENCE  %03d%%  %s", (int)(opening_progress * 100.0f), (step % 2) ? "_" : " ");
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        return true;
    }

    static void ChangeStyle()
    {
        if (!ImGui::flag_change_thema)
            return;

        ImGui::flag_change_thema = false;

        if (ImGui::theme_id == 0) ImGui::ThemeWhite();
        else                      ImGui::ThemeDark();
    }

}
