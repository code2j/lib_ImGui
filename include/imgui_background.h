#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <iomanip>
#include <string>
#include <queue>
#include <iostream>


#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"


class ImGuiBackground {
public:
    // 백그라운드 스레드에서 ImGui 앱 시작
    static void init(const std::string& title, const ImVec2& size = ImVec2(1280, 720));

    // ImGui 앱 종료
    static void destroy();

    // 렌더링 콜백 설정 (매 프레임마다 호출됨)
    static void context(std::function<void()> func);

    // ImGui 앱이 실행 중인지 확인
    static bool is_running();

    // 이미지 불러오기
    static void load_image(const std::string& path, GLuint& texture_id, int& width, int& height);

    // 이미지 리소스 해제
    static void release_image(GLuint& texture_id);

    // OpenGL 컨텍스트에 작업 푸시
    static void context_push(std::function<void()> func);

    // imgui 설정 파일 경로 설정 (기본은 실행 파일 위치)
    static void set_config_path(const std::string& path);


private:
    // 디폴트 윈도우 크기, 타이틀 이름
    inline static std::string TITLE                   = "DEMO";             // 윈도우 타이틀
    inline static float WINDOW_WIDTH                  = 1280;               // width
    inline static float WINDOW_HEIGHT                 = 720;                // height


    // 폰트 관련 상수
    inline static const float FONST_SIZE              = 28.0f;              // 폰트 크기
    inline static const float ICON_SIZE               = 30.0f;              // 아이콘 폰트 크기
    inline static const ImVec2 GLYPH_OFFSET           = ImVec2(-3.5, 3.0);  // 폰트 오프셋


    // 타이틀바 관련 상수
    inline static const ImVec2 TITLEBAR_PADDING       = ImVec2(0, 0.0f);    // 타이틀바 패딩
    inline static const ImVec2 TITLEBAR_BUTTON_OFFSET = ImVec2(-32, 0);     // 타이틀바 버튼 오프셋
    inline static const ImVec2 TITLEBAR_BUTTON_SIZE   = ImVec2(30.6, 30.6);     // 타이틀바 버튼 사이즈


    // 독스페이스 배경 색상, 투명도 상수
    inline static ImVec4 CLEAR_COLOR                  = ImVec4(0.1f, 0.1f, 0.1f, 0.7f);
    inline static float DOCKSPACE_MARGIN              = 1.0f;
    inline static float TITLEBAR_HEIGHT               = 30.0f;


    // 도킹 위치 이동 및 크기 조절 변수
    inline static ImVec2 docking_size                 = ImVec2(WINDOW_WIDTH - DOCKSPACE_MARGIN, WINDOW_HEIGHT - DOCKSPACE_MARGIN);
    inline static ImVec2 prev_docking_size            = docking_size;
    inline static ImVec2 docking_pos                  = ImVec2(0, 0);


    // 타이틀바 드래깅관련 변수
    inline static bool is_dragging_titlebar           = false;
    inline static double drag_offset_x                = 0.0;
    inline static double drag_offset_y                = 0.0;



    // ===================================================
    // 기능 함수
    // ===================================================
    bool _init();                     // 초기화
    bool _imgui_rendering_loop();     // 렌더링 루프

    void _start_background();         // 백그라운드 스레드에서 시작
    void _stop_background();          // 백그라운드 스레드 종료


    // ===================================================
    // GUI 함수
    // ===================================================
    void _show_dockspace(); // 메인 독스페이스 GUI
    void _show_titlebar();  // 커스텀 타이틀바 GUI



    // 싱글톤
    ImGuiBackground() = default;
    ImGuiBackground(const ImGuiBackground&) = delete;
    ImGuiBackground& operator=(const ImGuiBackground&) = delete;

    static ImGuiBackground& getInstance();


    // 렌더링 콜백
    std::function<void()> render_callback = nullptr;
    std::mutex callback_mutex;

    std::queue<std::function<void()>> context_queue;

    std::thread render_thread;
    std::atomic<bool> _is_running{false};

    std::string config_path_ = "../config/imgui.ini";
};


