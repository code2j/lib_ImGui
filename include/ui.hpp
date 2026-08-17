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
#include "ui_axis.h"
#include "ui_logger.h"
#include "ui_notify.h"
#include "ui_loader.h"
#include "ui_points.h"
#include "ui_widgets.h"


#define SKYBOX_ON 1  // 1: 스카이 박스 보임, 0: 스카이박스 안보임

namespace ImGuiExt
{
    // ==================================================
    // 상태 플래그
    // ==================================================
    inline bool should_close_app = false; // 앱 닫기 여부
    inline bool show_3d_viewport = false; // 3d 뷰포트 보이기 여부
    inline bool show_log_window  = false; // 로거 표시 여부
    inline bool show_style_edit  = false; // 컬러 에디터 표시 여부
    inline int  theme_id         = 1;     // 0: white, 1: dark


    // ==================================================
    // 폰트 객체
    // ==================================================
    inline ImFont* D2Cording = nullptr;


    // =====================================================
    // 쉐이더
    // =====================================================
    inline Shader shader_instancing;


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
    bool should_close();
    bool context(std::function<void()> func);   // Gui 실행 컨텍스트
    void load_config(const char* path);         // config 파일 불러오기
    void help(const char* desc);


    // =====================================================
    // 테마 스타일
    // =====================================================
    void style_white();
    void style_dark();


    // =====================================================
    // 뷰포트 마우스 및 상태 유틸리티
    // =====================================================
    Vector2 get_viewport_mouse_pos(); // 뷰포트의 마우스 위치
    bool    is_viewport_hovered();
}


