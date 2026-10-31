#pragma once
#include <string>
#include <functional>
#include <GLFW/glfw3.h>

//
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

// ImGui 핵심 헤더
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot3d.h"
#include "implot.h"

#include "ui_ini.h"
#include "ui_font_icon.h"
#include "ui_3d.h"
#include "ui_logger.h"
#include "ui_notify.h"
#include "ui_loader.h"
#include "ui_points.h"
#include "ui_widgets.h"
#include "ui_system_hud.h"
#include "ui_style.h"


namespace ImGui
{
    // ==================================================
    // 상태 플래그
    // ==================================================
    inline bool should_close_app   = false; // 앱 닫기 여부
    inline bool show_3d_viewport   = false; // 3d 뷰포트 보이기 여부
    inline bool show_log_window    = false; // 로거 표시 여부
    inline bool show_style_edit    = false; // 컬러 에디터 표시 여부
    inline bool show_system_hud    = false; // hud 표시 여부
    inline int  theme_id           = 1;     // 0: white, 1: dark
    inline bool flag_change_thema  = false; // 테마 변경 여부


    // ==================================================
    // 폰트 객체
    // ==================================================
    inline ImFont* LOGGER_FONT      = nullptr;


    // ==================================================
    // 기본 함수 정의
    // ==================================================
    void Init(const char* title, int width = 1280, int height = 720);
    void Destroy();
    bool ShouldClose(bool force_close = false);
    bool Context(std::function<void()> func);
    void LoadConfig(const char* path);
    void AddTab(const char* label, std::function<void()> draw = {}, std::function<void()> sidebar_content = {});
}