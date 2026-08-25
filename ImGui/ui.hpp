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
#include "ui_icon.h"
#include "ui_draw3d.h"
#include "ui_logger.h"
#include "ui_notify.h"
#include "ui_loader.h"
#include "ui_points.h"
#include "ui_widgets.h"
#include "ui_system_hud.h"
#include "ui_style.h"


#define SKYBOX_ON 0  // 1: 스카이 박스 보임, 0: 스카이박스 안보임

namespace ImGui
{
    inline ImVec2 GLOBAL_MOUSE_POS;       // 전역 마우스 좌표


    // ==================================================
    // 상태 플래그
    // ==================================================
    inline bool should_close_app = false; // 앱 닫기 여부
    inline bool show_3d_viewport = false; // 3d 뷰포트 보이기 여부
    inline bool show_log_window  = false; // 로거 표시 여부
    inline bool show_style_edit  = false; // 컬러 에디터 표시 여부
    inline bool show_system_hud  = false; // hud 표시 여부
    inline bool show_main_menu   = false; // 메인 메뉴 표시 여부

    inline int  theme_id         = 1;     // 0: white, 1: dark

    inline bool flag_change_thema  = false;
    inline bool flag_load_complete = false;
    inline bool flag_show_menu     = false;



    // ==================================================
    // 폰트 객체
    // ==================================================
    inline ImFont* D2Cording = nullptr;



    // ==================================================
    // 3d 카메라 정의
    // ==================================================
    inline Camera camera = {
        { 1.0, 0.5, 2.0 },      // position
        { 0.0, 0.0, 0.0 },       // target
        { 0.0, 1.0, 0.0 },       // up
        45.0f,                   // fovy
        CAMERA_PERSPECTIVE       // projection
    };
}



namespace ImGui
{
    void init(const char* title, int width = 1280, int height = 720);
    void destroy();
    bool should_close(bool force_close = false);
    bool context(std::function<void()> func);
    void load_config(const char* path);
    void show_menu(bool b);


    // =====================================================
    // 뷰포트 마우스 및 상태 유틸리티
    // =====================================================
    Vector2 get_viewport_mouse_pos(); // 뷰포트의 마우스 위치
    bool    is_viewport_hovered();
}


