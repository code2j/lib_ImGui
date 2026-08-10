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

#include "ui_icon.h"
#include "ui_axis.h"
#include "ui_logger.h"
#include "ui_notify.h"
#include "ui_toggle.h"
#include "ui_loader.h"


#define SKYBOX_ON 1  // 1: 스카이 박스 보임, 0: 스카이박스 안보임

namespace ImGuiExt
{
    // ==================================================
    // 상태 플래그
    // ==================================================
    inline bool show_3d_viewport = false; // 3d 뷰포트 보이기 여부
    inline bool should_close_app = false; // 앱 닫기 여부
    inline bool show_log_window  = false; // 로거 표시 여부


    // ==================================================
    // 폰트 객체
    // ==================================================
    inline ImFont* D2Cording = nullptr;

}



namespace ImGui
{
    void init(const char* title, int width = 1280, int height = 720);
    void destroy();
    bool should_close();
    void context(std::function<void()> func);
    void load_config(const std::string& path);


    // =====================================================
    // 뷰포트 마우스 및 상태 유틸리티
    // =====================================================
    Vector2 get_viewport_mouse_pos(); // 뷰포트의 마우스 위치
    bool    is_viewport_hovered();


    // =====================================================
    // ImGui ini 핸들러 콜백 함수 정의
    // =====================================================
    void* read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);
    void read_line(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);
    void write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
}


