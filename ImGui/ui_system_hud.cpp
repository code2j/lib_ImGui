#include "ui_system_hud.h"
#include "ui_icon.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

namespace {
    struct CpuData {
        unsigned long long prev_idle = 0;
        unsigned long long prev_total = 0;
        float usage = 0.0f;
    };

    // 전체 및 코어별 CPU 사용량 업데이트 함수
    void update_cpu_usage(CpuData& total_cpu, std::vector<CpuData>& core_cpus)
    {
        std::ifstream file("/proc/stat");
        std::string line;

        int core_idx = 0;
        while (std::getline(file, line)) {
            // "cpu"로 시작하는 줄만 파싱 (전체 cpu 및 cpu0, cpu1 등)
            if (line.compare(0, 3, "cpu") != 0) break;

            std::istringstream ss(line);
            std::string cpu_label;
            unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
            ss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            unsigned long long current_idle = idle + iowait;
            unsigned long long current_non_idle = user + nice + system + irq + softirq + steal;
            unsigned long long current_total = current_idle + current_non_idle;

            CpuData* target = nullptr;
            if (cpu_label == "cpu") {
                target = &total_cpu; // 첫 번째 줄은 전체 사용량
            } else {
                // 코어 개수에 맞춰 백터 크기 동적 할당
                if (core_idx >= core_cpus.size()) core_cpus.resize(core_idx + 1);
                target = &core_cpus[core_idx];
                core_idx++;
            }

            if (target) {
                unsigned long long total_diff = current_total - target->prev_total;
                unsigned long long idle_diff = current_idle - target->prev_idle;

                if (total_diff > 0) {
                    target->usage = static_cast<float>(total_diff - idle_diff) / total_diff * 100.0f;
                }

                target->prev_idle = current_idle;
                target->prev_total = current_total;
            }
        }
    }
}

void ImGui::draw_system_hud()
{
    static CpuData total_cpu;
    static std::vector<CpuData> core_cpus;
    static float ram_usage = 0.0f;
    static float ram_used_gb = 0.0f;
    static float ram_total_gb = 0.0f;
    static double last_time = 0.0;

    const double update_time = 0.5;

    double current_time = ImGui::GetTime();

    // 데이터 갱신 로직 (기존과 동일)
    if (current_time - last_time > update_time) {
        update_cpu_usage(total_cpu, core_cpus);

        std::ifstream mem_file("/proc/meminfo");
        std::string mem_line;
        long long mem_total = 0, mem_available = 0;

        while (std::getline(mem_file, mem_line)) {
            if (mem_line.compare(0, 8, "MemTotal") == 0) {
                std::istringstream iss(mem_line);
                std::string key, unit;
                iss >> key >> mem_total >> unit;
            } else if (mem_line.compare(0, 12, "MemAvailable") == 0) {
                std::istringstream iss(mem_line);
                std::string key, unit;
                iss >> key >> mem_available >> unit;
            }
        }

        if (mem_total > 0) {
            long long mem_used = mem_total - mem_available;
            ram_usage = static_cast<float>(mem_used) / mem_total * 100.0f;
            ram_used_gb = static_cast<float>(mem_used) / (1024.0f * 1024.0f);
            ram_total_gb = static_cast<float>(mem_total) / (1024.0f * 1024.0f);
        }

        last_time = current_time;
    }

    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    static bool show_cores          = false;
    const float window_width        = 160.0f;
    const float margin_right        = 15.0f;
    const float margin_top          = 15.0f;
    const float gap_between_windows = 8.0f;  // 창 사이의 간격
    const float alpha               = 0.4;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));


    // ==========================================
    // 1. 메인 HUD 창
    // ==========================================
    ImGui::SetNextWindowSize(ImVec2(window_width, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - window_width - margin_right, margin_top), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha);

    float main_window_height = 0.0f;

    if (ImGui::Begin("Simple CPU HUD Main", nullptr, window_flags)) {

        // CPU 텍스트
        ImGui::Text(ICON_MD_MEMORY " CPU: %.1f%%", total_cpu.usage);
        if (ImGui::IsItemHovered()) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY()));
            ImGui::SetTooltip("Click to Show All Core");
        }
        if (ImGui::IsItemClicked()) {
            show_cores = !show_cores;
        }


        // RAM 텍스트
        ImGui::Text(ICON_MD_STORAGE " RAM: %.1f%%", ram_usage);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%.1f GB/%.1f GB", ram_used_gb, ram_total_gb);
        }


        // 메인 창의 최종 높이를 저장
        main_window_height = ImGui::GetWindowHeight();
    }
    ImGui::End();


    // ------------------------------------------
    //  세부 정보 창 (CPU 클릭 시)
    // ------------------------------------------
    if (show_cores && !core_cpus.empty()) {
        float details_pos_y = margin_top + main_window_height + gap_between_windows;

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - window_width - margin_right, details_pos_y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(window_width, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(alpha);

        if (ImGui::Begin("Simple CPU HUD Details", nullptr, window_flags)) {

            float max_usage = 0.0f;
            float min_usage = 100.0f;
            for (const auto& core : core_cpus) {
                if (core.usage > max_usage) max_usage = core.usage;
                if (core.usage < min_usage) min_usage = core.usage;
            }
            float range = max_usage - min_usage;
            if (range <= 0.001f) range = 1.0f;

            if (ImGui::BeginTable("CoreUsageTable", 2, ImGuiTableFlags_None)) {
                ImGui::TableSetupColumn("CoreLabel", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("UsageValue", ImGuiTableColumnFlags_WidthFixed, 55.0f); // 여백 조정

                for (size_t i = 0; i < core_cpus.size(); ++i) {
                    float t = (core_cpus[i].usage - min_usage) / range;
                    float r = 0.8f + (0.2f * t);
                    float g = 0.8f - (0.5f * t);
                    float b = 0.8f - (0.5f * t);
                    ImVec4 color = ImVec4(r, g, b, 1.0f);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "Core %zu", i);

                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%5.1f%%", core_cpus[i].usage);
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    // 스타일 롤백 (다른 ImGui 창에 영향이 가지 않도록 해제)
    ImGui::PopStyleVar(3);

}
