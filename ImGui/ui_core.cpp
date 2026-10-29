#define IMGUI_DEFINE_MATH_OPERATORS
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
#include <vector>
namespace fs = std::filesystem;

namespace
{
    bool window_pinned = false;
    int previous_window_focus = -1;
    Window previous_active_window = None;
    std::string config_path;
    float ui_content_scale = 1.0f;
    struct SidebarTab { std::string label; std::function<void()> draw; };
    std::vector<SidebarTab> sidebar_tabs;

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
    void AddTab(const char* label, std::function<void()> draw)
    {
        if (!label || !*label) return;
        for (auto& tab : sidebar_tabs) {
            if (tab.label == label) { tab.draw = std::move(draw); return; }
        }
        sidebar_tabs.push_back({label, std::move(draw)});
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

        // 설정 팝업
        ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f * scale, 0), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10) * scale);
        if (ImGui::BeginPopup("SettingsPopup")) {
            ImGui::Spacing();
            ImGui::SeparatorText("패널 표시");
            bool config_changed = ImGui::MenuItem(ICON_MD_DEPLOYED_CODE "  3D Viewport", nullptr, &ImGui::show_3d_viewport);
            config_changed |= ImGui::MenuItem(ICON_MD_SUBJECT "  Log", nullptr, &ImGui::show_log_window);
            ImGui::MenuItem(ICON_MD_TUNE "  Style Editor", nullptr, &ImGui::show_style_edit);
            config_changed |= ImGui::MenuItem("System HUD", nullptr, &ImGui::show_system_hud);
            ImGui::Spacing();
            ImGui::SeparatorText("화면 테마");
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

        // 최소화 버튼
        if (action(2, ICON_MD_REMOVE "##window_minimaze", "최소화"))
            MinimizeWindow();

        // 닫기 버튼
        if (action(3, ICON_MD_CLOSE "##window_close", "닫기", false, true))
            ImGui::ShouldClose(true);

        // 좌측 패널과 도킹 영역을 나란히 배치한다.
        const float body_y = header_height + 8.0f * scale;
        const float sidebar_width = ImMin(220.0f * scale, size.x * 0.32f);
        ImGui::SetCursorPos(ImVec2(8.0f * scale, body_y + 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8) * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6) * scale);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));
        ImGui::BeginChild("##sidebar", ImVec2(sidebar_width, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
        // ImGui::TextUnformatted("Workspace");
        // ImGui::TextDisabled("NAVIGATION");
        ImGui::Dummy(ImVec2(0, 0.0f * scale));
        static int selected_page = 0;
        const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        const ImVec4 subtle_accent = ImVec4(accent.x, accent.y, accent.z, 0.28f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.08f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
        const int page_count = (int)sidebar_tabs.size();
        if (selected_page >= page_count) selected_page = 0;
        static float indicator_page = 0.0f;
        ImVec2 first_button_min;
        for (int i = 0; i < page_count; ++i) {
            const bool selected = selected_page == i;
            ImGui::PushStyleColor(ImGuiCol_Button, selected ? subtle_accent : ImVec4(0, 0, 0, 0));
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
            ImGui::PushID(i);
            const char* label = sidebar_tabs[i].label.c_str();
            if (ImGui::Button(label, ImVec2(-FLT_MIN, 42.0f * scale))) selected_page = i;
            if (i == 0) first_button_min = ImGui::GetItemRectMin();
            ImGui::PopID();
            if (selected) ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }
        if (page_count > 0) {
            // 페이지 인덱스 기준으로 보간하면 창 이동과 화면 배율 변경에도 위치가 유지된다.
            indicator_page += (selected_page - indicator_page) *
                              (1.0f - std::exp(-12.0f * ImGui::GetIO().DeltaTime));
            const float width = 4.0f * scale;
            const float inset = 7.0f * scale;
            const float y = first_button_min.y + indicator_page * 48.0f * scale;
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(first_button_min.x, y + inset),
                ImVec2(first_button_min.x + width, y + 42.0f * scale - inset),
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
        all_dockspaces.reserve(sidebar_tabs.size());
        for (const auto& tab : sidebar_tabs) {
            const std::string id = "SidebarDockSpace:" + tab.label;
            all_dockspaces.push_back(ImGui::GetID(id.c_str()));
        }
        const ImVec2 dockspace_pos = ImGui::GetCursorScreenPos();
        for (int i = 0; i < page_count; ++i)
            if (i != selected_page) {
                ImGuiDockNode* node = ImGui::DockBuilderGetNode(all_dockspaces[i]);
                if (!node || !node->HostWindow) {
                    // A keep-alive-only dockspace cannot create its host on startup.
                    // Create it once so restored windows stay hidden on inactive pages.
                    ImGui::SetCursorScreenPos(dockspace_pos);
                    ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_None);
                }
                ImGui::DockSpace(all_dockspaces[i], ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
            }
        ImGui::SetCursorScreenPos(dockspace_pos);
        if (page_count > 0)
            ImGui::DockSpace(all_dockspaces[selected_page], ImVec2(0, 0), ImGuiDockNodeFlags_None);
        ImGui::PopStyleColor();
        ImGui::End();
        if (page_count > 0) {
            auto draw_tab = sidebar_tabs[selected_page].draw;
            if (draw_tab) draw_tab();
        }
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
        sidebar_tabs.clear();
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
