#include "ui_system_hud.h"
#include "ui_icon.h"
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdio> // popen, pclose 사용

namespace {
    struct CpuData {
        unsigned long long prev_idle = 0;
        unsigned long long prev_total = 0;
        float usage = 0.0f;
        float temp = -1.0f;
    };

    struct RamData {
        float usage = 0.0f;
        float used_gb = 0.0f;
        float total_gb = 0.0f;
    };

    struct GpuData {
        float usage = 0.0f;
        float mem_used_gb = 0.0f;
        float mem_total_gb = 0.0f;
        float temp = -1.0f;
        bool available = false;
    };

    // 1. CPU 사용량 업데이트
    void update_cpu_usage(CpuData& total_cpu, std::vector<CpuData>& core_cpus)
    {
        std::ifstream file("/proc/stat");
        std::string line;
        int core_idx = 0;
        while (std::getline(file, line)) {
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
                target = &total_cpu;
            } else {
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

    // 2. CPU 온도 업데이트
    void update_cpu_temp(CpuData& total_cpu)
    {
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        long millicelsius = 0;
        if (temp_file >> millicelsius) {
            total_cpu.temp = millicelsius / 1000.0f;
        } else {
            total_cpu.temp = -1.0f;
        }
    }

    // 3. RAM 사용량 업데이트
    void update_ram_usage(RamData& ram)
    {
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
            ram.usage = static_cast<float>(mem_used) / mem_total * 100.0f;
            ram.used_gb = static_cast<float>(mem_used) / (1024.0f * 1024.0f);
            ram.total_gb = static_cast<float>(mem_total) / (1024.0f * 1024.0f);
        }
    }

    // 4. GPU 사용량 및 온도 업데이트 (NVIDIA)
    void update_gpu_usage(GpuData& gpu)
    {
        FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
        if (!pipe) {
            gpu.available = false;
            return;
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string result = buffer;
            std::replace(result.begin(), result.end(), ',', ' ');
            std::istringstream iss(result);

            float util, mem_used_mib, mem_total_mib, temp;
            if (iss >> util >> mem_used_mib >> mem_total_mib >> temp) {
                gpu.usage = util;
                gpu.mem_used_gb = mem_used_mib / 1024.0f;
                gpu.mem_total_gb = mem_total_mib / 1024.0f;
                gpu.temp = temp;
                gpu.available = true;
            } else {
                gpu.available = false;
            }
        } else {
            gpu.available = false;
        }

        pclose(pipe);
    }
}

void ImGui::draw_system_hud(bool open)
{
    if (!open) {
        return;
    }

    static CpuData total_cpu;
    static std::vector<CpuData> core_cpus;
    static RamData ram;
    static GpuData gpu;
    static double last_time = 0.0;

    const double update_time = 0.5;
    double current_time = ImGui::GetTime();

    if (current_time - last_time > update_time) {
        update_cpu_usage(total_cpu, core_cpus);
        update_cpu_temp(total_cpu);
        update_ram_usage(ram);
        update_gpu_usage(gpu);

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

    const float window_width        = 260.0f;
    const float margin_right        = 15.0f;
    const float margin_top          = 15.0f;
    const float gap_between_windows = 8.0f;
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

    if (ImGui::Begin("Simple System HUD Main", nullptr, window_flags)) {

        if (ImGui::BeginTable("MainHUDTable", 2, ImGuiTableFlags_None)) {
            // [수정됨] 첫 번째 열(라벨)의 고정 폭을 55.0f -> 75.0f 로 늘려 아이콘과 글자가 겹치지 않게 함
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            // --- [CPU Row] ---
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text(ICON_MD_MEMORY " CPU");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click to Show All Core");
            if (ImGui::IsItemClicked()) show_cores = !show_cores;

            ImGui::TableNextColumn();
            char cpu_val[64];
            if (total_cpu.temp > 0.0f) snprintf(cpu_val, sizeof(cpu_val), "%.1f%% (%.1f ℃)", total_cpu.usage, total_cpu.temp);
            else snprintf(cpu_val, sizeof(cpu_val), "%.1f%%", total_cpu.usage);

            float text_width = ImGui::CalcTextSize(cpu_val).x;
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, avail - text_width));
            ImGui::TextUnformatted(cpu_val);


            // --- [RAM Row] ---
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text(ICON_MD_STORAGE " RAM");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("USAGE: %.1f GB/%.1f GB", ram.used_gb, ram.total_gb);

            ImGui::TableNextColumn();
            char ram_val[64];
            snprintf(ram_val, sizeof(ram_val), "%.1f%%", ram.usage);

            text_width = ImGui::CalcTextSize(ram_val).x;
            avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, avail - text_width));
            ImGui::TextUnformatted(ram_val);


            // --- [GPU Row] ---
            if (gpu.available) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text(ICON_MD_MEMORY " GPU");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("VRAM: %.1f GB/%.1f GB", gpu.mem_used_gb, gpu.mem_total_gb);

                ImGui::TableNextColumn();
                char gpu_val[64];
                if (gpu.temp > 0.0f) snprintf(gpu_val, sizeof(gpu_val), "%.1f%% (%.1f ℃)", gpu.usage, gpu.temp);
                else snprintf(gpu_val, sizeof(gpu_val), "%.1f%%", gpu.usage);

                text_width = ImGui::CalcTextSize(gpu_val).x;
                avail = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, avail - text_width));
                ImGui::TextUnformatted(gpu_val);
            }

            ImGui::EndTable();
        }

        main_window_height = ImGui::GetWindowHeight();
    }
    ImGui::End();

    // ------------------------------------------
    // 2. 세부 정보 창 (CPU 클릭 시)
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
                // [수정됨] "Core XX" 텍스트 공간 확보
                ImGui::TableSetupColumn("CoreLabel", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableSetupColumn("UsageValue", ImGuiTableColumnFlags_WidthStretch);

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
                    char core_val[32];
                    snprintf(core_val, sizeof(core_val), "%5.1f%%", core_cpus[i].usage);

                    float text_width = ImGui::CalcTextSize(core_val).x;
                    float avail = ImGui::GetContentRegionAvail().x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, avail - text_width));

                    ImGui::TextColored(color, "%s", core_val);
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    ImGui::PopStyleVar(3);
}