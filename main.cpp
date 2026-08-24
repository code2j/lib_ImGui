#include "ui.hpp"
#include <iostream>

#define FILE_PCD "aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();

#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#if __has_include(<nvml.h>)
#include <nvml.h>
#define HAS_NVML 1
#else
#define HAS_NVML 0
#endif

struct SystemMetrics {
    float cpu_usage = 0.0f;
    
    // RAM 정보
    float ram_usage_pct = 0.0f;
    float ram_used_gb = 0.0f;
    float ram_total_gb = 0.0f;

    // GPU 정보
    bool gpu_available = false;
    std::string gpu_name = "N/A";
    float gpu_usage_pct = 0.0f;
    float vram_used_gb = 0.0f;
    float vram_total_gb = 0.0f;
    unsigned int gpu_temp = 0;
};

// 1. CPU 사용량 계산 (/proc/stat)
float get_cpu_usage() {
    static uint64_t prev_idle = 0, prev_total = 0;
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0f;

    std::string cpu;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    uint64_t idle_ticks = idle + iowait;
    uint64_t total_ticks = user + nice + system + idle + iowait + irq + softirq + steal;

    uint64_t diff_idle = idle_ticks - prev_idle;
    uint64_t diff_total = total_ticks - prev_total;

    prev_idle = idle_ticks;
    prev_total = total_ticks;

    if (diff_total == 0) return 0.0f;
    return 1.0f - (static_cast<float>(diff_idle) / static_cast<float>(diff_total));
}

// 2. RAM 사용량 파싱 (/proc/meminfo)
void get_ram_usage(SystemMetrics& metrics)
{
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return;

    std::string line;
    uint64_t total_kb = 0, avail_kb = 0;

    while (std::getline(file, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::sscanf(line.c_str(), "MemTotal: %lu kB", &total_kb);
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            std::sscanf(line.c_str(), "MemAvailable: %lu kB", &avail_kb);
        }
    }

    if (total_kb > 0) {
        uint64_t used_kb = total_kb - avail_kb;
        metrics.ram_usage_pct = static_cast<float>(used_kb) / static_cast<float>(total_kb);
        metrics.ram_used_gb = static_cast<float>(used_kb) / (1024.0f * 1024.0f);
        metrics.ram_total_gb = static_cast<float>(total_kb) / (1024.0f * 1024.0f);
    }
}

// 3. GPU 사용량 정보 수집 (NVIDIA NVML / AMD sysfs 하이브리드)
void get_gpu_usage(SystemMetrics& metrics) {
#if HAS_NVML
    static bool nvml_inited = false;
    static nvmlDevice_t device;

    if (!nvml_inited) {
        if (nvmlInit() == NVML_SUCCESS) {
            if (nvmlDeviceGetHandleByIndex(0, &device) == NVML_SUCCESS) {
                nvml_inited = true;
                char name[64];
                if (nvmlDeviceGetName(device, name, sizeof(name)) == NVML_SUCCESS) {
                    metrics.gpu_name = name;
                }
            }
        }
    }

    if (nvml_inited) {
        metrics.gpu_available = true;

        // 사용률
        nvmlUtilization_t util;
        if (nvmlDeviceGetUtilizationRates(device, &util) == NVML_SUCCESS) {
            metrics.gpu_usage_pct = static_cast<float>(util.gpu) / 100.0f;
        }

        // VRAM
        nvmlMemory_t mem;
        if (nvmlDeviceGetMemoryInfo(device, &mem) == NVML_SUCCESS) {
            metrics.vram_used_gb = static_cast<float>(mem.used) / (1024.0f * 1024.0f * 1024.0f);
            metrics.vram_total_gb = static_cast<float>(mem.total) / (1024.0f * 1024.0f * 1024.0f);
        }

        // 온도
        unsigned int temp = 0;
        if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
            metrics.gpu_temp = temp;
        }
        return;
    }
#endif

    // AMD GPU Sysfs Fallback (/sys/class/drm/card0/device/gpu_busy_percent)
    std::ifstream amd_gpu("/sys/class/drm/card0/device/gpu_busy_percent");
    if (amd_gpu.is_open()) {
        int usage = 0;
        amd_gpu >> usage;
        metrics.gpu_available = true;
        metrics.gpu_name = "AMD GPU";
        metrics.gpu_usage_pct = static_cast<float>(usage) / 100.0f;
    }
}

// 오버레이 HUD 렌더링
void DrawSystemHud() {
    static SystemMetrics metrics;
    static std::vector<float> cpu_history(40, 0.0f);
    static std::vector<float> gpu_history(40, 0.0f);
    static auto last_update = std::chrono::steady_clock::now();

    // 0.5초 주기 측정
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() > 500) {
        metrics.cpu_usage = get_cpu_usage();
        get_ram_usage(metrics);
        get_gpu_usage(metrics);

        // 히스토리 그래프 갱신
        for (size_t i = 0; i < cpu_history.size() - 1; ++i) {
            cpu_history[i] = cpu_history[i + 1];
            gpu_history[i] = gpu_history[i + 1];
        }
        cpu_history.back() = metrics.cpu_usage * 100.0f;
        gpu_history.back() = metrics.gpu_usage_pct * 100.0f;

        last_update = now;
    }

    // 투명 오버레이 패널 플래그
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_AlwaysAutoResize | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoFocusOnAppearing | 
                             ImGuiWindowFlags_NoNav;

    // 우측 상단 고정 위치 계산
    const float margin = 12.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - margin, viewport->WorkPos.y + margin);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.65f); // 65% 반투명

    if (ImGui::Begin("System Performance HUD", nullptr, flags)) {
        char buf[64];

        // 1. CPU Section
        ImGui::Text(ICON_MD_MEMORY " CPU Usage");
        snprintf(buf, sizeof(buf), "%.1f %%", metrics.cpu_usage * 100.0f);
        ImGui::ProgressBar(metrics.cpu_usage, ImVec2(220.0f, 0.0f), buf);
        ImGui::PlotLines("##CPU_Plot", cpu_history.data(), static_cast<int>(cpu_history.size()), 0, nullptr, 0.0f, 100.0f, ImVec2(220.0f, 30.0f));

        ImGui::Dummy(ImVec2(0, 4.0f));
        ImGui::Separator();

        // 2. RAM Section
        ImGui::Text(ICON_MD_STORAGE " RAM Usage");
        snprintf(buf, sizeof(buf), "%.1f / %.1f GB (%.0f%%)", metrics.ram_used_gb, metrics.ram_total_gb, metrics.ram_usage_pct * 100.0f);
        ImGui::ProgressBar(metrics.ram_usage_pct, ImVec2(220.0f, 0.0f), buf);

        // 3. GPU Section (감지된 경우)
        if (metrics.gpu_available) {
            ImGui::Dummy(ImVec2(0, 4.0f));
            ImGui::Separator();

            ImGui::Text(ICON_MD_SPEED " GPU (%s)", metrics.gpu_name.c_str());
            if (metrics.gpu_temp > 0) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%u°C)", metrics.gpu_temp);
            }

            snprintf(buf, sizeof(buf), "%.1f %%", metrics.gpu_usage_pct * 100.0f);
            ImGui::ProgressBar(metrics.gpu_usage_pct, ImVec2(220.0f, 0.0f), buf);
            ImGui::PlotLines("##GPU_Plot", gpu_history.data(), static_cast<int>(gpu_history.size()), 0, nullptr, 0.0f, 100.0f, ImVec2(220.0f, 30.0f));

            if (metrics.vram_total_gb > 0.0f) {
                ImGui::TextDisabled("VRAM: %.1f / %.1f GB", metrics.vram_used_gb, metrics.vram_total_gb);
            }
        }
    }
    ImGui::End();
}



int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");

    {
        // 스코프 안에서 생성하면 자동으로 해제됨
        ImGui::Texture texture1 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO1.png");

        while (ImGui::context([&]() {
            DrawSystemHud();

            ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

            if (ImGui::BeginCollapsingHeader(ICON_MD_WIDGETS " Widgets Example ")) {
                // Child 창 시작
                ImGui::BeginChild("Widgets_Child", ImVec2(0, 1300), true);

                // ==========================================
                // 1. 기본 컨트롤 (Basic Controls)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_CHECK_BOX " Basic Controls");
                ImGui::Separator();

                static bool is_c = false;
                ImGui::Check("체크박스 (커스텀)", &is_c);
                ImGui::Dummy(ImVec2(0, 15.0f));


                static int radio_idx = 0;
                ImGui::Radio("라디오 1 (커스텀)", &radio_idx, 0);
                ImGui::Radio("라디오 2 (커스텀)", &radio_idx, 1);
                ImGui::Dummy(ImVec2(0, 15.0f));

                if (ImGui::Button("일반 버튼", ImVec2(100, 0))) {
                    // 버튼 클릭 이벤트
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                if (ImGui::Button("위험 버튼", ImVec2(100, 0))) {
                    // 경고/삭제 버튼 이벤트
                }
                ImGui::PopStyleColor(3);
                ImGui::Dummy(ImVec2(0, 15.0f));


                // ==========================================
                // 2. 텍스트 입력 (Text Inputs)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_EDIT " Text Inputs");
                ImGui::Separator();

                static char text_buf[128] = "Hello, Custom ImGui!";
                ImGui::InputText("텍스트 입력", text_buf, IM_COUNTOF(text_buf));

                static char pw_buf[64] = "";
                ImGui::InputText("비밀번호", pw_buf, IM_COUNTOF(pw_buf), ImGuiInputTextFlags_Password);

                static char multiline_buf[256] = "여기에\n여러 줄의\n텍스트를 입력하세요.";
                ImGui::InputTextMultiline("메모", multiline_buf, IM_COUNTOF(multiline_buf), ImVec2(0, 60));
                ImGui::Dummy(ImVec2(0, 15.0f));



                // ==========================================
                // 3. 슬라이더 및 드래그 (Sliders & Drags)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TUNE " Sliders & Drags");
                ImGui::Separator();

                static double drag_value = 50.0f;
                ImGui::Drag("드래그 (커스텀)", &drag_value, 1.0f, 0.0f, 100.0f, "%.1f");

                static float slider_value = 30.0f;
                ImGui::SliderFloatX("슬라이더 FloatX", &slider_value, 0.0f, 100.0f, "%.1f");
                ImGui::SliderX("슬라이더 X", &slider_value, 0.0f, 100.0f, "%.1f");
                ImGui::Slider("슬라이더 (값 표시 숨김)", &slider_value, 0.0f, 100.0f);

                static float range_min = 20.0f, range_max = 80.0f;
                ImGui::SliderRangeX("범위 슬라이더 X", &range_min, &range_max, 0.0f, 100.0f);
                ImGui::SliderRange("범위 슬라이더 (기본)", &range_min, &range_max, 0.0f, 100.0f);
                ImGui::Dummy(ImVec2(0, 15.0f));

                // ==========================================
                // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
                ImGui::Separator();

                static int current_theme = 1;
                ImGui::DropDown("테마 선택 (DropDown)", &current_theme, "Light Theme\0Dark Theme\0Classic Theme\0", 3);

                static int listbox_item_current = 1;
                const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange" };
                ImGui::ListBox("과일 목록", &listbox_item_current, listbox_items, IM_COUNTOF(listbox_items), 4);
                ImGui::Dummy(ImVec2(0, 15.0f));

                // ==========================================
                // 5. 색상 선택기 (Color Pickers)
                // ==========================================
                ImGui::Dummy(ImVec2(0, 15.0f));
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_COLOR_LENS " Color Pickers");
                ImGui::Separator();

                static float color_rgb[3] = { 0.36f, 0.41f, 0.94f };
                ImGui::ColorEdit3("테마 색상", color_rgb);

                static float color_rgba[4] = { 0.8f, 0.2f, 0.3f, 0.5f };
                ImGui::ColorEdit4("알파 포함 색상", color_rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
                ImGui::Dummy(ImVec2(0, 15.0f));

                // ==========================================
                // 6. 상태 및 진행률 (Misc / Progress)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_INFO " Status & Misc");
                ImGui::Separator();

                static float progress = 0.0f;
                progress += 0.005f; // 애니메이션 효과를 위해 임의 증가
                if (progress > 1.0f) progress = 0.0f;

                ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "로딩 중...");

                ImGui::TextWrapped("위젯 위로 마우스를 올리면 툴팁을 확인할 수 있습니다.");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("이것은 ImGui 표준 툴팁입니다.");
                }

                ImGui::EndChild();
                ImGui::EndCollapsingHeader();
            }


            if (ImGui::BeginCollapsingHeader(ICON_MD_CHAT_INFO " Notification Example ")) {
                // [알림 버튼 샘플]
                if (ImGui::Button(" 알림 정보 ")) {
                    ImGui::NotifyInfo("정보 알림이 표시 됩니다.");
                    std::cout << "[Info ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 성공 ")) {
                    ImGui::NotifySucc("성공 알림이 표시 됩니다.");
                    std::cout << "[Succ ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 경고 ")) {
                    ImGui::NotifyWarn("경고 알림이 표시 됩니다.");
                    std::cout << "[Warn ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 에러 ")) {
                    ImGui::NotifyError("에러 알림이 표시 됩니다.");
                    std::cout << "[Error] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }

                ImGui::EndCollapsingHeader();
            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_TUNE " Slider Example ")) {
                // [드래그 슬라이더 샘플]
                ImGui::BeginChild("##slider", ImVec2(0, 190), true);
                ImGui::PushItemWidth(300);

                static float drag = 10.0f;
                ImGui::DragFloat("Drag", &drag);
                ImGui::Dummy(ImVec2(0, 1));

                static float slider = 0.314f;
                ImGui::SliderFloat("Slider", &slider, 0.0f, 1.0f);
                ImGui::Dummy(ImVec2(0, 1));

                static float sliderx = 0.314f;
                ImGui::SliderX("SlideXr", &sliderx, 0.0f, 1.0f);
                ImGui::Dummy(ImVec2(0, 1));

                // [범위 슬라이더 샘플]
                static float price_min = 0.0f;
                static float price_max = 10.0f;
                ImGui::SliderRange("Range", &price_min, &price_max, 0.0f, 10.0f, "%.1f");
                ImGui::Dummy(ImVec2(0, 1));

                static int cnt = 1.0;
                if (ImGui::InputInt("Input", &cnt)) {
                    cnt = std::max(static_cast<int>(price_min), std::min(cnt, static_cast<int>(price_max)));
                }
                ImGui::Dummy(ImVec2(0, 1));

                ImGui::PopItemWidth();
                ImGui::EndChild();

                ImGui::EndCollapsingHeader();
            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_JOYSTICK " Joystick Example ")) {
                // [조이스틱 샘플]
                ImVec2 joy;
                ImGui::Joystic(&joy);
                ImGui::Dummy(ImVec2(0, 20));
                ImGui::EndCollapsingHeader();
            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_STEPPERS " Status Step Example ", false)) {
                const char* status_labels[] = {
                    "시스템 시작",
                    "시스템 초기화",
                    "시스템 준비",
                    "시스템 동작",
                };

                static int current_account_status = 2;

                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                ImGui::BeginChild("상태바", ImVec2(0, 200), true);

                // 1. 스테이터스 바 렌더링 (상단 위치)
                ImGui::StatusStepBar("##AccountStatusStepBar", &current_account_status, status_labels, 4);

                ImGui::Dummy(ImVec2(0, 10.0f)); // 바 위젯과의 여백 확보

                // 2. InputInt 위젯 가로 중앙 정렬 처리
                float input_width = 140.0f; // InputInt의 너비 지정
                float avail_width = ImGui::GetContentRegionAvail().x;
                float offset_x = (avail_width - input_width) * 0.5f;

                if (offset_x > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

                ImGui::PushItemWidth(input_width);
                if (ImGui::InputInt("##상태", &current_account_status)) {
                    current_account_status = std::max(0, std::min(current_account_status, 3));
                }
                ImGui::PopItemWidth();

                ImGui::EndChild();
                ImGui::PopStyleVar();

                ImGui::EndCollapsingHeader();
            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_IMAGE " Image Example ")) {
                // 토클 이미지 보이기
                static bool show_image = false;
                if (ImGui::Toggle("이미지 보이기", &show_image)) {
                    ImGui::NotifyInfo("이미지가 표시됩니다.");
                }
                ImGui::SameLine();
                ImGui::Text("이미지 보이기");


                ImGui::BeginChild("child", ImVec2(0, 400), true);
                if (show_image) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float img_aspect = (float) texture1->width / (float) texture1->height;
                    float avail_aspect = avail.x / avail.y;

                    ImVec2 size;
                    if (avail_aspect > img_aspect) {
                       // 가용 영역이 더 넓음 - 높이에 맞춤
                       size.y = avail.y;
                       size.x = size.y * img_aspect;
                    } else {
                       // 가용 영역이 더 좁음 - 너비에 맞춤
                       size.x = avail.x;
                       size.y = size.x / img_aspect;
                    }

                    ImGui::Image(texture1->id, size);
                }
                ImGui::EndChild();
                ImGui::EndCollapsingHeader();

            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_COLORS " Theme Example ")) {

                static int current_theme = 1;

                ImGui::BeginChild("ThemeSelector", ImVec2(0, 150), true);
                if (ImGui::ThemeSelector(&ImGuiExt::theme_id))
                {
                    // 테마가 변경되었을 때 실행할 로직
                    if (ImGuiExt::theme_id == 0) {
                        ImGui::style_white();
                    }
                    else {
                        ImGui::style_dark();
                    }

                }
                ImGui::EndChild();

                ImGui::EndCollapsingHeader();
            }






            ImGui::End();



            ImGui::ShowDemoWindow();


            ImGui::Begin(" " ICON_MD_GAMEPAD " TF 컨트롤 ");
            ImGui::TransformControl(&tf_control);
            ImGui::End();
        }));
    }


    ImGui::destroy();
    return 0;
}


