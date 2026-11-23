#define IMGUI_DEFINE_MATH_OPERATORS
#include "ui.hpp"
#include "ui_icon.cpp"
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
#include <algorithm>
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
    ImGuiID selected_dockspace = 0;


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
        ImGui3D::Destroy3D();
        DestroyAllTextures();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImPlot3D::DestroyContext();
        ImGui::DestroyContext();
        CloseWindow();
        ImGui::ShutdownLogger();
    }

    ImGuiID GetSelectedDockspace() { return selected_dockspace; }

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

        ImGui3D::ClearFrame();

        ChangeStyle();


        if (!FakeLoading()) {
            ImGui::ShowMenu();
            ImGui::ShowLogger(" " ICON_MD_SUBJECT " Log ", ImGui::show_log_window);
            ImGui::ShowNotifications();
            ImGui::ShowSystemHud(ImGui::show_system_hud);
            if (ImGui::show_style_edit) {
                ImGui::ShowStyleEditor(&ImGui::GetStyle());
            }

            if (func) func();
        }


        // 다른 창이 클릭으로 앞으로 이동한 뒤에도 오버레이의 순서를 유지한다.
        ImGuiWindow* settings_button = ImGui::FindWindowByName("##floating_settings");
        ImGuiWindow* settings_panel = ImGui::FindWindowByName("##settings_panel");
        if (settings_button && settings_button->Active)
            ImGui::BringWindowToDisplayFront(settings_button);
        if (settings_panel && settings_panel->Active)
            ImGui::BringWindowToDisplayFront(settings_panel);

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
                tab.content_height = 0.0f;
                return;
            }
        }
        SIDETAP_BAR.push_back({label, std::move(draw), std::move(sidebar_content)});
    }

    static void GetDesktopWorkArea(ImVec2& position, ImVec2& size)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        position = viewport->WorkPos;
        size = viewport->WorkSize;
        Display* display = glfwGetX11Display();
        Window root = DefaultRootWindow(display);
        Atom type = None;
        int format = 0;
        unsigned long count = 0, remaining = 0;
        unsigned char* data = nullptr;
        unsigned long desktop = 0;
        Atom current = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
        if (XGetWindowProperty(display, root, current, 0, 1, False, XA_CARDINAL,
                               &type, &format, &count, &remaining, &data) == Success &&
            type == XA_CARDINAL && format == 32 && count == 1)
            desktop = *reinterpret_cast<unsigned long*>(data);
        if (data) XFree(data);
        data = nullptr;
        Atom workArea = XInternAtom(display, "_NET_WORKAREA", False);
        if (XGetWindowProperty(display, root, workArea, desktop * 4, 4, False, XA_CARDINAL,
                               &type, &format, &count, &remaining, &data) != Success ||
            type != XA_CARDINAL || format != 32 || count != 4) {
            if (data) XFree(data);
            return;
        }
        const auto* area = reinterpret_cast<unsigned long*>(data);
        const long workX = static_cast<int32_t>(area[0]);
        const long workY = static_cast<int32_t>(area[1]);
        const long workRight = workX + static_cast<long>(area[2]);
        const long workBottom = workY + static_cast<long>(area[3]);
        XFree(data);
        XWindowAttributes attrs{};
        int rootX = 0, rootY = 0;
        Window child = None;
        if (!XGetWindowAttributes(display, glfwGetX11Window(glfwGetCurrentContext()), &attrs) ||
            !XTranslateCoordinates(display, glfwGetX11Window(glfwGetCurrentContext()), root, 0, 0, &rootX, &rootY, &child) ||
            attrs.width <= 0 || attrs.height <= 0) return;
        const long left = std::max<long>(rootX, workX);
        const long top = std::max<long>(rootY, workY);
        const long right = std::min<long>(rootX + attrs.width, workRight);
        const long bottom = std::min<long>(rootY + attrs.height, workBottom);
        if (right <= left || bottom <= top) return;
        const float scaleX = viewport->Size.x / attrs.width;
        const float scaleY = viewport->Size.y / attrs.height;
        position = ImVec2(viewport->Pos.x + (left - rootX) * scaleX,
                          viewport->Pos.y + (top - rootY) * scaleY);
        size = ImVec2((right - left) * scaleX, (bottom - top) * scaleY);
    }

    static void ShowMenu()
    {
        // 화면 배율에 맞춰 상단 헤더와 버튼의 크기 및 간격을 정한다.
        const float scale = ui_content_scale;
        const float header_height = 76.0f * scale;
        const float button_size = 38.0f * scale;
        const float gap = 6.0f * scale;
        const float inset = 16.0f * scale;
        static bool maximized = false;
        static ImVec2 restore_position, restore_size;
        const ImGuiWindowFlags flags = (maximized ? ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoScrollWithMouse |
                                       ImGuiWindowFlags_NoScrollbar;

        // 기본 윈도우
        if (maximized) {
            ImVec2 work_position, work_size;
            GetDesktopWorkArea(work_position, work_size);
            ImGui::SetNextWindowPos(work_position);
            ImGui::SetNextWindowSize(work_size);
        } else {
            ImGui::SetNextWindowSize(ImVec2(1000, 700) * scale, ImGuiCond_FirstUseEver);
        }
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
        static bool sidebar_open = true;
        static float sidebar_expansion = 1.0f;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, pos + ImVec2(size.x, header_height),
                            ImGui::GetColorU32(ImGuiCol_ChildBg), 12.0f * scale,
                            ImDrawFlags_RoundCornersTop);
        const ImVec4 accent_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        // 포인터의 가로 위치를 따라 빛이 이동하고, 창 밖에서는 서서히 사라진다.
        static float highlight_x = 0.5f;
        static float highlight_alpha = 0.0f;
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool mouse_inside = mouse.x >= pos.x && mouse.x < pos.x + size.x &&
                                  mouse.y >= pos.y && mouse.y < pos.y + size.y;
        const float blend = 1.0f - std::exp(-5.0f * ImGui::GetIO().DeltaTime);
        if (mouse_inside && size.x > 0.0f)
            highlight_x = ImLerp(highlight_x, ImClamp((mouse.x - pos.x) / size.x, 0.0f, 1.0f), blend);
        highlight_alpha = ImLerp(highlight_alpha, mouse_inside ? 1.0f : 0.0f, blend);
        if (highlight_alpha > 0.001f) {
            const float center = pos.x + highlight_x * size.x;
            const float radius = 160.0f * scale;
            const float top = pos.y + header_height - 3.0f * scale;
            const float bottom = pos.y + header_height;
            const ImU32 clear = ImGui::GetColorU32(ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.0f));
            const ImU32 glow = ImGui::GetColorU32(ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.38f * highlight_alpha));
            draw->PushClipRect(pos, pos + ImVec2(size.x, header_height), true);
            draw->AddRectFilledMultiColor(ImVec2(center - radius, top), ImVec2(center, bottom), clear, glow, glow, clear);
            draw->AddRectFilledMultiColor(ImVec2(center, top), ImVec2(center + radius, bottom), glow, clear, clear, glow);
            draw->PopClipRect();
        }
        draw->AddLine(pos + ImVec2(0, header_height), pos + ImVec2(size.x, header_height),
                      ImGui::GetColorU32(ImGuiCol_Border));

        // 해더 영역 드래깅
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##title_drag", ImVec2(actions_x - gap, header_height));
        if (!maximized && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            ImGui::SetWindowPos(pos + ImGui::GetIO().MouseDelta);

        // 헤더 왼쪽에 위젯 아이콘 배지와 창 제목, WORKSPACE 부제를 그린다.
        const ImVec2 badge = pos + ImVec2(inset + (60.0f * scale - button_size) * 0.5f, 19.0f * scale);
        draw->AddRectFilled(badge, badge + ImVec2(button_size, button_size), ImGui::GetColorU32(ImGuiCol_Button), 10.0f * scale);
        const ImVec2 icon_size = ImGui::CalcTextSize(ICON_MD_WIDGETS);
        draw->AddText(badge + (ImVec2(button_size, button_size) - icon_size) * 0.5f, IM_COL32(255, 255, 255, 255), ICON_MD_WIDGETS);
        const float text_x = badge.x - pos.x + button_size + 12.0f * scale;
        draw->PushClipRect(pos + ImVec2(text_x, 0), pos + ImVec2(actions_x - gap, header_height), true);
        draw->AddText(pos + ImVec2(text_x, 17.0f * scale), ImGui::GetColorU32(ImGuiCol_Text), WINDOW_TITLE.c_str());
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.72f, pos + ImVec2(text_x, 43.0f * scale), ImGui::GetColorU32(ImGuiCol_TextDisabled), "WORKSPACE");
        draw->PopClipRect();

        // 공통 버튼 스타일
        auto action = [&](int index, const char* icon, const char* tooltip, bool selected = false, bool destructive = false)
        {
            ImGui::SetCursorPos(ImVec2(actions_x + index * (button_size + gap), 19.0f * scale));
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

        if (action(0, ICON_MD_KEEP "##window_pin", window_pinned ? "고정 해제" : "항상 위에 고정", window_pinned)) {
            window_pinned = !window_pinned;
            update_window_stacking();
        }
        if (action(1, ICON_MD_REMOVE "##window_minimaze", "최소화")) MinimizeWindow();
        if (action(2, maximized ? ICON_MD_FULLSCREEN_EXIT "##maximize" : ICON_MD_FULLSCREEN "##maximize",
                   maximized ? "복원" : "최대화")) {
            if (!maximized) {
                restore_position = pos;
                restore_size = size;
                maximized = true;
            } else {
                maximized = false;
                ImGui::SetWindowPos(restore_position);
                ImGui::SetWindowSize(restore_size);
            }
        }
        if (action(3, ICON_MD_CLOSE "##window_close", "닫기", false, true)) ImGui::ShouldClose(true);

        // 사이드 패널 영역 설정
        const float body_y = header_height + 8.0f * scale;
        sidebar_expansion += ((sidebar_open ? 1.0f : 0.0f) - sidebar_expansion) *
                             (1.0f - std::exp(-14.0f * ImGui::GetIO().DeltaTime));
        if (std::abs(sidebar_expansion - (sidebar_open ? 1.0f : 0.0f)) < 0.001f)
            sidebar_expansion = sidebar_open ? 1.0f : 0.0f;
        const float expanded_width = ImMin(220.0f * scale, size.x * 0.32f);
        const float sidebar_width = SIDETAP_BAR.empty() ? 0.0f : 60.0f * scale + (expanded_width - 60.0f * scale) * sidebar_expansion;
        const bool compact_sidebar = sidebar_expansion < 0.8f;
        static int selected_page = 0;
        const int page_count = (int)SIDETAP_BAR.size();
        if (!SIDETAP_BAR.empty()) {
        ImGui::SetCursorPos(ImVec2(16.0f * scale, body_y + 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6) * scale);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
        ImGui::BeginChild("##sidebar", ImVec2(sidebar_width, -8.0f * scale), ImGuiChildFlags_AlwaysUseWindowPadding);

        // 사이드 패널 버튼 공통 스타일 설정
        const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 subtle_accent = ImVec4(accent.x, accent.y, accent.z, 0.40f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.08f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));

        if (selected_page >= page_count) selected_page = 0;

        // 하단 버튼 공간을 확보하고 메뉴 항목만 스크롤한다.
        ImGui::BeginChild("##sidebar_items", ImVec2(0, -54.0f * scale), ImGuiChildFlags_None);

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
            if (ImGui::Button("##sidebar_tab", ImVec2(-FLT_MIN, 48.0f * scale))) {
                if (!compact_sidebar && tab.sidebar_content)
                    tab.open = selected ? !tab.open : true;
                selected_page = i;
            }
            const ImVec2 button_min = ImGui::GetItemRectMin();
            const ImVec2 button_max = ImGui::GetItemRectMax();
            unsigned int codepoint = 0;
            const int first_len = ImTextCharFromUtf8(&codepoint, label, nullptr);
            const char* label_end = ImGui::FindRenderedTextEnd(label);
            const char* first_end = label + (first_len > 0 ? first_len : 0);
            if (first_end > label_end) first_end = label_end;
            const ImVec2 icon_size = ImGui::CalcTextSize(label, first_end);
            const float icon_x = button_min.x + (44.0f * scale - icon_size.x) * 0.5f;
            const float text_y = button_min.y + (button_max.y - button_min.y - icon_size.y) * 0.5f;
            ImDrawList* button_draw = ImGui::GetWindowDrawList();
            button_draw->PushClipRect(button_min, button_max, true);
            button_draw->AddText(ImVec2(icon_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), label, first_end);
            if (first_end < label_end && sidebar_expansion > 0.0f)
                button_draw->AddText(ImVec2(icon_x + icon_size.x, text_y), ImGui::GetColorU32(ImGuiCol_Text, sidebar_expansion), first_end, label_end);
            button_draw->PopClipRect();
            if (compact_sidebar && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("%s", label);
            if (selected_page == i) {
                target_y = button_min.y - sidebar_origin_y;
                indicator_x = button_min.x;
            }
            ImGui::PopID();
            if (selected) ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            // 하위 내용의 높이를 먼저 측정한 뒤 크기와 투명도를 함께 변경한다.
            if (tab.sidebar_content && !compact_sidebar) {
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
        ImGui::EndChild();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::Button("##sidebar_toggle", ImVec2(-FLT_MIN, 48.0f * scale)))
            sidebar_open = !sidebar_open;
        const ImVec2 toggle_min = ImGui::GetItemRectMin();
        const ImVec2 toggle_max = ImGui::GetItemRectMax();
        const char* toggle_icon = sidebar_open ? ICON_MD_MENU_OPEN : ICON_MD_MENU;
        const ImVec2 toggle_icon_size = ImGui::CalcTextSize(toggle_icon);
        const float toggle_x = toggle_min.x + (44.0f * scale - toggle_icon_size.x) * 0.5f;
        const float toggle_y = toggle_min.y + (toggle_max.y - toggle_min.y - toggle_icon_size.y) * 0.5f;
        ImDrawList* toggle_draw = ImGui::GetWindowDrawList();
        toggle_draw->PushClipRect(toggle_min, toggle_max, true);
        toggle_draw->AddText(ImVec2(toggle_x, toggle_y), ImGui::GetColorU32(ImGuiCol_TextDisabled), toggle_icon);
        if (sidebar_expansion > 0.0f)
            toggle_draw->AddText(ImVec2(toggle_min.x + 44.0f * scale, toggle_y),
                                 ImGui::GetColorU32(ImGuiCol_TextDisabled, sidebar_expansion), "사이드바 접기");
        toggle_draw->PopClipRect();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", sidebar_open ? "사이드바 접기" : "사이드바 펼치기");
        ImGui::PopStyleColor();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        }

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
                    ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_NoWindowMenuButton);
                }
                ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly | ImGuiDockNodeFlags_NoWindowMenuButton);
            }
        // 선택된 탭의 도킹 영역과 내용을 표시한다.
        ImGui::SetCursorScreenPos(dockspace_pos);
        selected_dockspace = page_count > 0 ? all_dockspaces[selected_page] : ImGui::GetID("DefaultDockSpace");
        ImGui::DockSpace(selected_dockspace, ImVec2(0, 0), ImGuiDockNodeFlags_NoWindowMenuButton);
        ImGui::PopStyleColor();
        ImGui::End();
        if (page_count > 0) {
            auto draw_tab = SIDETAP_BAR[selected_page].draw;
            if (draw_tab) draw_tab();
        }
    // 별도 오버레이 창으로 도킹된 콘텐츠 위에 설정 버튼을 유지한다.
    const float floating_size = 52.0f * scale;
    const float floating_margin = 20.0f * scale;
    static float floating_hover = 0.0f;
    ImGui::SetNextWindowPos(pos + size - ImVec2(floating_size + floating_margin, floating_size + floating_margin));
    ImGui::SetNextWindowSize(ImVec2(floating_size, floating_size));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, floating_size * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin("##floating_settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground);
    static float settings_open_progress = 0.0f;
    static bool settings_open = false;
    // 바깥 클릭은 애니메이션과 그리기 전에 처리해 같은 프레임부터 닫는다.
    if (settings_open && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGuiWindow* panel = ImGui::FindWindowByName("##settings_panel");
        if (panel && panel->WasActive && !ImGui::IsMouseHoveringRect(panel->Pos, panel->Pos + panel->Size, false))
            settings_open = false;
    }
    if (ImGui::InvisibleButton("##window_setting", ImVec2(floating_size, floating_size))) {
        settings_open = true;
    }
    // 버튼과 패널이 같은 애니메이션 값을 사용해 전환 중 빈 프레임이 생기지 않게 한다.
    const float animation_step = ImGui::GetIO().DeltaTime / (settings_open ? 0.42f : 0.30f);
    settings_open_progress = ImClamp(settings_open_progress + (settings_open ? animation_step : -animation_step), 0.0f, 1.0f);
    const bool hovered = ImGui::IsItemHovered();
    floating_hover += ((hovered ? 1.0f : 0.0f) - floating_hover) * (1.0f - std::exp(-10.0f * ImGui::GetIO().DeltaTime));
    const float eased = floating_hover * floating_hover * (3.0f - 2.0f * floating_hover);
    const ImVec2 center = ImGui::GetItemRectMin() + ImVec2(floating_size, floating_size) * 0.5f;
    const ImVec4 base = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    const ImVec4 highlight = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    const ImVec4 color = ImLerp(base, highlight, eased);
    // 포커스 변경으로 창 정렬이 바뀌어도 버튼이 가려지지 않도록 최상단에 그린다.
    ImDrawList* floating_draw = ImGui::GetForegroundDrawList();
    const float progress = settings_open_progress;
    const float button_alpha = settings_open_progress > 0.0f ? 0.0f : 1.0f;
    const float icon_fade = ImClamp(progress / 0.35f, 0.0f, 1.0f);
    const float icon_alpha = 1.0f - icon_fade * icon_fade * (3.0f - 2.0f * icon_fade);
    const float pressed = ImGui::IsItemActive() ? 1.5f : 0.0f;
    const float radius = (22.0f + 2.0f * eased - pressed) * scale;
    floating_draw->AddCircleFilled(center + ImVec2(0, 2.0f * scale), radius,
                                   ImGui::GetColorU32(ImVec4(0, 0, 0, 0.18f * button_alpha)));
    floating_draw->AddCircleFilled(center, radius, ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * button_alpha)));
    floating_draw->AddCircle(center, radius - scale,
                            ImGui::GetColorU32(ImVec4(1, 1, 1, (0.12f + 0.22f * eased) * button_alpha)), 0, scale);
    const ImVec2 settings_icon_size = ImGui::CalcTextSize(ICON_MD_SETTINGS);
    floating_draw->AddText(center - settings_icon_size * 0.5f, ImGui::GetColorU32(ImGuiCol_Text, icon_alpha), ICON_MD_SETTINGS);
    if (hovered && !settings_open && settings_open_progress == 0.0f) ImGui::SetTooltip("워크스페이스 설정");
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    if (settings_open_progress > 0.0f) {
        const float p = settings_open_progress;
        const float t = p * p * p * (p * (p * 6.0f - 15.0f) + 10.0f);
        // 버튼의 중심에서 패널이 자라나며 우하단 모서리에 정착한다.
        const ImVec2 button_center = pos + size - ImVec2(floating_margin + floating_size * 0.5f, floating_margin + floating_size * 0.5f);
        const ImVec2 panel_corner = pos + size - ImVec2(floating_margin, floating_margin);
        // 먼저 가로로 펼친 뒤 세로로 확장해 버튼이 패널로 변하는 느낌을 준다.
        const float width_t = 1.0f - (1.0f - t) * (1.0f - t);
        const float height_t = t * t * (3.0f - 2.0f * t);
        // 테마 카드는 고정 픽셀 크기이므로 배율이 작아도 두 카드가 들어가야 한다.
        const float panel_width = ImMax(328.0f * scale, 296.0f + 32.0f * scale);
        static float measured_height = 0.0f;
        static float measured_scale = 0.0f;
        if (measured_scale != scale) {
            measured_scale = scale;
            measured_height = 0.0f;
        }
        const float panel_height = measured_height > 0.0f ? measured_height : 440.0f * scale;
        const float button_diameter = radius * 2.0f;
        const ImVec2 animated_size(ImLerp(button_diameter, panel_width, width_t),
                                   ImLerp(button_diameter, panel_height, height_t));
        const ImVec2 animated_corner = ImLerp(button_center + ImVec2(radius, radius), panel_corner, t);
        ImGui::SetNextWindowPos(animated_corner, ImGuiCond_Always, ImVec2(1, 1));
        ImGui::SetNextWindowSize(animated_size);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ImLerp(floating_size * 0.5f, 12.0f * scale, t));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10) * scale);
        const ImVec4 window_bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        const float luminance = window_bg.x * 0.2126f + window_bg.y * 0.7152f + window_bg.z * 0.0722f;
        const ImVec4 panel_bg = ImLerp(window_bg, luminance < 0.5f ? ImVec4(1, 1, 1, 1) : ImVec4(1, 1, 1, 1), luminance < 0.5f ? 0.07f : 0.65f);
        // 패널을 별도 표시 레이어에 두어 바깥 창이 포커스를 얻어도 가려지지 않게 한다.
        ImGui::PushStyleColor(ImGuiCol_WindowBg, panel_bg);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, ImLerp(floating_size * 0.5f, 12.0f * scale, t));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f * scale);
        ImGui::Begin("##settings_panel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoBackground);
        // 배경과 테두리를 매 프레임 다시 그려 크기, 둥글기, 색상이 함께 변하도록 한다.
        ImDrawList* panel_draw = ImGui::GetWindowDrawList();
        const ImVec2 panel_min = ImGui::GetWindowPos();
        const ImVec2 panel_max = panel_min + ImGui::GetWindowSize();
        const float rounding = ImLerp(radius, 12.0f * scale, t);
        const ImVec4 morph_color = ImLerp(color, panel_bg, t);
        panel_draw->PushClipRect(panel_min, panel_max, false);
        panel_draw->AddRectFilled(panel_min, panel_max, ImGui::GetColorU32(morph_color), rounding);
        const ImVec4 border = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        panel_draw->AddRect(panel_min + ImVec2(0.5f, 0.5f) * scale,
                            panel_max - ImVec2(0.5f, 0.5f) * scale,
                            ImGui::GetColorU32(ImLerp(base, border, t)), rounding, 0, scale);
        panel_draw->PopClipRect();
            const float content_alpha = ImClamp((t - 0.55f) / 0.45f, 0.0f, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * content_alpha);
            ImGui::TextUnformatted("Settings");
            ImGui::SameLine(ImGui::GetWindowWidth() - 16.0f * scale - 28.0f * scale);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            if (ImGui::Button(ICON_MD_CLOSE "##close_settings", ImVec2(28.0f, 28.0f) * scale)) settings_open = false;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            ImGui::TextUnformatted("Menu");
            ImGui::Separator();
            bool config_changed = false;
            config_changed |= ImGui::MenuItem(ICON_MD_DEPLOYED_CODE "  3D Viewport", nullptr, &ImGui::open_viewport);
            config_changed |= ImGui::MenuItem(ICON_MD_SUBJECT "  Log", nullptr, &ImGui::show_log_window);
            config_changed |= ImGui::MenuItem(ICON_MD_TUNE "  Style Editor", nullptr, &ImGui::show_style_edit);
            config_changed |= ImGui::MenuItem(ICON_MD_POSITION_TOP_RIGHT "  System HUD", nullptr, &ImGui::show_system_hud);
            ImGui::Spacing();
            ImGui::TextUnformatted("Theme");
            ImGui::Separator();
            if (ImGui::ThemeSelector(&ImGui::theme_id)) {
                ImGui::flag_change_thema = true;
                config_changed = true;
            }
            if (config_changed && ImGui::GetIO().IniFilename)
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            // 마지막 항목까지의 실제 높이에 하단 여백을 더해 다음 프레임의 크기로 사용한다.
            if (settings_open && settings_open_progress >= 1.0f)
                measured_height = ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y + 16.0f * scale;
        ImGui::PopStyleVar();
        ImGui::End();
        ImGui::PopStyleVar(6);
        ImGui::PopStyleColor();
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
