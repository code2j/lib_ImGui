#include "ui.hpp"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include <future>
#include <vector>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <limits>
#include <cmath>

#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();





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


static void ShowTelemetryHud(const ImVec2& origin, const ImVec2& area)
{
    if (area.x <= 0.0f || area.y <= 0.0f) return;
    static CpuData cpu;
    static std::vector<CpuData> cores;
    static RamData ram;
    static GpuData gpu;
    static std::future<GpuData> gpu_request;
    static double last_sample = -1.0;
    const double now = ImGui::GetTime();
    if (gpu_request.valid() && gpu_request.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        gpu = gpu_request.get();
    if (last_sample < 0.0 || now - last_sample >= 1.0) {
        update_cpu_usage(cpu, cores);
        update_ram_usage(ram);
        if (!gpu_request.valid()) gpu_request = std::async(std::launch::async, [] {
            GpuData sample;
            update_gpu_usage(sample);
            return sample;
        });
        last_sample = now;
    }

    const float scale = ImGui::GetFontSize() / 16.0f;
    const float margin = 16.0f * scale;
    const float radius = 21.0f * scale;
    const float spacing = 56.0f * scale;
    const float width = spacing * 2.0f + radius * 2.0f;
    const float height = radius * 2.0f + ImGui::GetTextLineHeight() + 8.0f * scale;
    if (area.x < width + margin * 2.0f || area.y < height + margin * 2.0f) return;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + area.x, origin.y + area.y), true);
    const ImVec2 first(origin.x + area.x - margin - width + radius,
                       origin.y + margin + radius);
    constexpr float pi = 3.14159265358979323846f;
    int hovered_gauge = -1;
    auto gauge = [&](int index, float value, bool available) {
        const ImVec2 center(first.x + index * spacing, first.y);
        const float fraction = std::clamp(value / 100.0f, 0.0f, 1.0f);
        const ImU32 accent = value >= 90.0f && available ? IM_COL32(255, 95, 91, 255) :
                             value >= 75.0f && available ? IM_COL32(255, 193, 91, 255) : IM_COL32(120, 224, 215, 255);
        draw->AddCircleFilled(center, radius - 3.0f * scale, IM_COL32(5, 17, 22, 165), 48);
        const float start = pi * 0.75f, end = pi * 2.25f;
        constexpr int segments = 15;
        for (int i = 0; i < segments; ++i) {
            const float segment = (end - start) / segments;
            const float a = start + segment * i + 0.04f;
            const float b = start + segment * (i + 1) - 0.04f;
            draw->PathArcTo(center, radius, a, b, 3);
            draw->PathStroke(available && (i + 1.0f) / segments <= fraction ? accent : IM_COL32(45, 93, 102, 150), 0, 3.0f * scale);
        }
        const ImVec2 mouse = ImGui::GetMousePos();
        const float dx = mouse.x - center.x, dy = mouse.y - center.y;
        if (ImGui::IsWindowHovered() && dx * dx + dy * dy <= radius * radius)
            hovered_gauge = index;
        const float x = center.x, y = center.y;
        const float icon_scale = scale * 1.45f;
        const float thickness = 2.0f * scale;
        const ImU32 icon = available ? IM_COL32(165, 239, 237, 255) : IM_COL32(99, 133, 136, 220);
        if (index == 0) { // CPU: 핀이 있는 칩
            draw->AddRect(ImVec2(x - 5 * icon_scale, y - 5 * icon_scale), ImVec2(x + 5 * icon_scale, y + 5 * icon_scale), icon, 0.0f, 0, thickness);
            draw->AddRect(ImVec2(x - 2 * icon_scale, y - 2 * icon_scale), ImVec2(x + 2 * icon_scale, y + 2 * icon_scale), icon, 0.0f, 0, thickness);
            for (int i = -1; i <= 1; ++i) {
                draw->AddLine(ImVec2(x + i * 3 * icon_scale, y - 7 * icon_scale), ImVec2(x + i * 3 * icon_scale, y - 5 * icon_scale), icon, thickness);
                draw->AddLine(ImVec2(x + i * 3 * icon_scale, y + 5 * icon_scale), ImVec2(x + i * 3 * icon_scale, y + 7 * icon_scale), icon, thickness);
            }
        } else if (index == 1) { // GPU: 팬이 달린 그래픽 카드
            draw->AddRect(ImVec2(x - 8 * icon_scale, y - 5 * icon_scale), ImVec2(x + 8 * icon_scale, y + 5 * icon_scale), icon, 0.0f, 0, thickness);
            draw->AddCircle(ImVec2(x, y), 3.0f * icon_scale, icon, 12, thickness);
            draw->AddCircleFilled(ImVec2(x, y), 1.0f * icon_scale, icon);
            draw->AddLine(ImVec2(x - 5 * icon_scale, y + 5 * icon_scale), ImVec2(x - 5 * icon_scale, y + 7 * icon_scale), icon, thickness);
            draw->AddLine(ImVec2(x + 5 * icon_scale, y + 5 * icon_scale), ImVec2(x + 5 * icon_scale, y + 7 * icon_scale), icon, thickness);
        } else { // RAM: 메모리 모듈
            draw->AddRect(ImVec2(x - 9 * icon_scale, y - 4 * icon_scale), ImVec2(x + 9 * icon_scale, y + 4 * icon_scale), icon, 0.0f, 0, thickness);
            for (int i = -1; i <= 1; ++i)
                draw->AddRectFilled(ImVec2(x + (i * 5 - 1) * icon_scale, y - 2 * icon_scale), ImVec2(x + (i * 5 + 1) * icon_scale, y + 1 * icon_scale), icon);
            draw->AddLine(ImVec2(x - 6 * icon_scale, y + 6 * icon_scale), ImVec2(x + 6 * icon_scale, y + 6 * icon_scale), icon, thickness);
        }
    };
    gauge(0, cpu.usage, cpu.prev_total != 0);
    gauge(1, gpu.usage, gpu.available);
    gauge(2, ram.usage, ram.total_gb > 0.0f);
    draw->PopClipRect();
    if (hovered_gauge >= 0) {
        const char* names[] = {"CPU", "GPU", "RAM"};
        const float values[] = {cpu.usage, gpu.usage, ram.usage};
        const bool available[] = {cpu.prev_total != 0, gpu.available, ram.total_gb > 0.0f};
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.86f, 0.92f, 0.9f));
        if (ImGui::BeginTooltip()) {
            ImDrawList* tooltip_draw = ImGui::GetWindowDrawList();
            tooltip_draw->ChannelsSplit(2);
            tooltip_draw->ChannelsSetCurrent(1);
            ImGui::TextColored(ImVec4(0.28f, 0.86f, 0.92f, 1.0f), "SYSTEM // %s", names[hovered_gauge]);
            ImGui::Separator();
            if (available[hovered_gauge]) ImGui::Text("사용률  %.1f%%", values[hovered_gauge]);
            else ImGui::TextUnformatted("사용률  N/A");
            if (hovered_gauge == 1) {
                if (gpu.available && gpu.mem_total_gb > 0.0f)
                    ImGui::Text("VRAM  %.1f / %.1f GiB", gpu.mem_used_gb, gpu.mem_total_gb);
                else ImGui::TextUnformatted("VRAM  N/A");
            } else if (hovered_gauge == 2) {
                if (ram.total_gb > 0.0f)
                    ImGui::Text("메모리  %.1f / %.1f GiB", ram.used_gb, ram.total_gb);
                else ImGui::TextUnformatted("메모리  N/A");
            }
            tooltip_draw->ChannelsSetCurrent(0);
            const ImVec2 tip_pos = ImGui::GetWindowPos();
            const ImVec2 cursor_max = ImGui::GetCursorScreenPos();
            const float tip_width = std::max(ImGui::GetWindowSize().x, ImGui::GetContentRegionMax().x + ImGui::GetStyle().WindowPadding.x);
            const float tip_height = cursor_max.y - tip_pos.y + ImGui::GetStyle().WindowPadding.y;
            const float cut = 8.0f;
            const ImVec2 frame[] = {ImVec2(tip_pos.x + cut, tip_pos.y), ImVec2(tip_pos.x + tip_width, tip_pos.y),
                ImVec2(tip_pos.x + tip_width, tip_pos.y + tip_height - cut), ImVec2(tip_pos.x + tip_width - cut, tip_pos.y + tip_height),
                ImVec2(tip_pos.x, tip_pos.y + tip_height), ImVec2(tip_pos.x, tip_pos.y + cut)};
            tooltip_draw->AddConvexPolyFilled(frame, 6, IM_COL32(6, 19, 26, 245));
            tooltip_draw->AddPolyline(frame, 6, IM_COL32(72, 220, 235, 230), ImDrawFlags_Closed, 1.0f);
            tooltip_draw->ChannelsMerge();
            ImGui::EndTooltip();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
}


namespace ImGui {
    // 현재 창의 지정 영역 중앙에 선택 패널을 표시한다.
        void CameraSelectionPanel(bool& open, const ImVec2& origin, const ImVec2& size)
    {
        ImGuiStorage* state = ImGui::GetStateStorage();
        const ImGuiID active_id = ImGui::GetID("##camera_selection_anim_active");
        const ImGuiID start_id = ImGui::GetID("##camera_selection_anim_start");
        if (!open) {
            state->SetBool(active_id, false);
            return;
        }
        if (!state->GetBool(active_id)) {
            state->SetBool(active_id, true);
            state->SetFloat(start_id, static_cast<float>(ImGui::GetTime()));
        }
        const float anim = ImClamp((static_cast<float>(ImGui::GetTime()) - state->GetFloat(start_id)) / 0.36f, 0.0f, 1.0f);
        if (open && size.x > 0 && size.y > 0) {
            ImGui::SetCursorScreenPos(origin);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.38f));
            ImGui::BeginChild("##camera_overlay", size, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            const ImVec2 card_size(std::min(380.0f, size.x), std::min(190.0f, size.y));
            const ImVec2 card_pos(origin.x + (size.x - card_size.x) * 0.5f,
                                  origin.y + (size.y - card_size.y) * 0.5f);
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const float cut = std::min(14.0f, std::min(card_size.x, card_size.y) * 0.25f);
            const ImVec2 corners[] = {ImVec2(card_pos.x + cut, card_pos.y),
                ImVec2(card_pos.x + card_size.x, card_pos.y),
                ImVec2(card_pos.x + card_size.x, card_pos.y + card_size.y - cut),
                ImVec2(card_pos.x + card_size.x - cut, card_pos.y + card_size.y),
                ImVec2(card_pos.x, card_pos.y + card_size.y), ImVec2(card_pos.x, card_pos.y + cut)};
            const float panel_alpha = ImClamp(anim / 0.30f, 0.0f, 1.0f);
            draw->AddConvexPolyFilled(corners, 6, IM_COL32(9, 24, 32, static_cast<int>(210 * panel_alpha)));
            draw->AddPolyline(corners, 6, IM_COL32(72, 220, 235, static_cast<int>(230 * panel_alpha)), ImDrawFlags_Closed, 1.0f);
            draw->AddRectFilled(ImVec2(card_pos.x, card_pos.y + cut), ImVec2(card_pos.x + 3, card_pos.y + card_size.y - 8), IM_COL32(72, 220, 235, static_cast<int>(255 * panel_alpha)));
            if (anim < 0.85f) {
                const float bracket_alpha = ImClamp((0.85f - anim) / 0.2f, 0.0f, 1.0f);
                const float offset = (1.0f - ImClamp(anim / 0.4f, 0.0f, 1.0f)) * 14.0f;
                const float length = std::min(18.0f, std::min(card_size.x, card_size.y) * 0.2f);
                const ImU32 bracket = IM_COL32(103, 243, 247, static_cast<int>(240 * bracket_alpha));
                for (int i = 0; i < 4; ++i) {
                    const float sx = (i & 1) ? -1.0f : 1.0f;
                    const float sy = (i & 2) ? -1.0f : 1.0f;
                    const ImVec2 p(card_pos.x + ((i & 1) ? card_size.x + offset : -offset),
                                   card_pos.y + ((i & 2) ? card_size.y + offset : -offset));
                    draw->AddLine(p, ImVec2(p.x + sx * length, p.y), bracket, 2.0f);
                    draw->AddLine(p, ImVec2(p.x, p.y + sy * length), bracket, 2.0f);
                }
            }
            if (anim >= 0.30f) {
            ImGui::SetCursorScreenPos(card_pos);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 14));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.30f, 0.36f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.43f, 0.50f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.54f, 0.60f, 1.0f));
            ImGui::BeginChild("##camera_selection", card_size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextColored(ImVec4(0.28f, 0.86f, 0.92f, 1.0f), "SYSTEM // SELECTION");
            ImGui::Separator();
            ImGui::TextUnformatted("선택하기");
            ImGui::TextDisabled("선택할 항목을 추가하세요.");
            ImGui::Dummy(ImVec2(0, 12));
            if (ImGui::Button("닫기", ImVec2(-1, 0))) open = false;
            ImGui::EndChild();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
            }
            ImGui::EndChild(); // ##camera_overlay
            ImGui::PopStyleColor();

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                open = false;
            if (!open) state->SetBool(active_id, false);
        }
    }


    // 라디얼 메뉴
    int RadialMenu(const char* id, const char* const items[], int count, float radius = 90.0f, float dead_zone = 20.0f, float hold_seconds = 0.25f)
    {
        if (count <= 0 || !items || radius <= 0.0f) return -1;
        dead_zone = std::max(0.0f, std::min(dead_zone, radius));
        ImGuiStorage* state = GetStateStorage();
        PushID(id);
        const ImGuiID open_id = GetID("##radial_open");
        const ImGuiID pending_id = GetID("##radial_pending");
        const ImGuiID time_id = GetID("##radial_start");
        const ImGuiID opened_at_id = GetID("##radial_opened_at");
        const ImGuiID x_id = GetID("##radial_x");
        const ImGuiID y_id = GetID("##radial_y");
        if (IsWindowHovered() && IsMouseClicked(ImGuiMouseButton_Right)) {
            const ImVec2 mouse = GetMousePos();
            state->SetBool(pending_id, true);
            state->SetBool(open_id, false);
            state->SetFloat(time_id, static_cast<float>(GetTime()));
            state->SetFloat(x_id, mouse.x);
            state->SetFloat(y_id, mouse.y);
        }
        if (state->GetBool(pending_id)) {
            if (!IsMouseDown(ImGuiMouseButton_Right) || IsKeyPressed(ImGuiKey_Escape)) {
                state->SetBool(pending_id, false);
            } else {
                const float elapsed = static_cast<float>(GetTime()) - state->GetFloat(time_id);
                const float progress = hold_seconds <= 0.0f ? 1.0f : std::min(1.0f, elapsed / hold_seconds);
                const ImVec2 center(state->GetFloat(x_id), state->GetFloat(y_id));
                ImDrawList* draw = GetForegroundDrawList();
                constexpr float pi = 3.14159265358979323846f;
                const float progress_angle = -pi * 0.5f + 2.0f * pi * progress;
                draw->AddCircleFilled(center, 14.0f, IM_COL32(9, 24, 32, 210), 48);
                draw->AddCircle(center, 11.0f, IM_COL32(63, 166, 183, 130), 48, 1.0f);
                if (progress < 1.0f) {
                    draw->PathArcTo(center, 17.0f, progress_angle, pi * 1.5f, 48);
                    draw->PathStroke(IM_COL32(38, 77, 87, 210), 0, 3.0f);
                }
                if (progress > 0.0f) {
                    draw->PathArcTo(center, 17.0f, -pi * 0.5f, progress_angle, 48);
                    draw->PathStroke(IM_COL32(72, 220, 235, 255), 0, 3.0f);
                    draw->AddCircleFilled(ImVec2(center.x + std::cos(progress_angle) * 17.0f,
                                                 center.y + std::sin(progress_angle) * 17.0f),
                                          2.5f, IM_COL32(172, 255, 255, 255));
                }
                draw->AddLine(ImVec2(center.x - 3, center.y), ImVec2(center.x + 3, center.y), IM_COL32(103, 243, 247, 220));
                draw->AddLine(ImVec2(center.x, center.y - 3), ImVec2(center.x, center.y + 3), IM_COL32(103, 243, 247, 220));
                if (progress >= 1.0f) {
                    state->SetBool(pending_id, false);
                    state->SetBool(open_id, true);
                    state->SetFloat(opened_at_id, static_cast<float>(GetTime()));
                }
            }
        }
        int result = -1;
        if (state->GetBool(open_id)) {
            constexpr float pi = 3.14159265358979323846f;
            const float step = 2.0f * pi / count;
            const ImVec2 center(state->GetFloat(x_id), state->GetFloat(y_id));
            const ImVec2 mouse = GetMousePos();
            const float dx = mouse.x - center.x, dy = mouse.y - center.y;
            int selected = -1;
            if (dx * dx + dy * dy >= dead_zone * dead_zone) {
                float angle = std::atan2(dy, dx) + pi * 0.5f + step * 0.5f;
                angle = std::fmod(angle + 2.0f * pi, 2.0f * pi);
                selected = static_cast<int>(angle / step) % count;
            }
            ImDrawList* draw = GetForegroundDrawList();
            const float reveal = std::min(1.0f, (static_cast<float>(GetTime()) - state->GetFloat(opened_at_id)) / 0.16f);
            const auto hud_color = [reveal](int r, int g, int b, int a = 255) {
                return IM_COL32(r, g, b, static_cast<int>(a * reveal));
            };
            for (int i = 0; i < count; ++i) {
                const float angle = -pi * 0.5f + i * step;
                draw->PathLineTo(center);
                draw->PathArcTo(center, radius, angle - step * 0.5f, angle + step * 0.5f, 32);
                draw->PathFillConvex(i == selected ? hud_color(18, 93, 108, 240) : hud_color(9, 24, 32, 235));
                const float inset = std::min(0.07f, step * 0.12f);
                draw->PathArcTo(center, std::max(0.0f, radius - 3.0f), angle - step * 0.5f + inset, angle + step * 0.5f - inset, 32);
                draw->PathStroke(i == selected ? hud_color(103, 243, 247) : hud_color(53, 150, 167, 170), 0, i == selected ? 3.0f : 1.0f);
                const ImVec2 size = CalcTextSize(items[i]);
                draw->AddText(ImVec2(center.x + std::cos(angle) * radius * 0.6f - size.x * 0.5f,
                                     center.y + std::sin(angle) * radius * 0.6f - size.y * 0.5f),
                              i == selected ? hud_color(172, 255, 255) : hud_color(194, 225, 227), items[i]);
            }
            for (int i = 0; i < count; ++i) {
                const float angle = -pi * 0.5f + (i - 0.5f) * step;
                const float x = std::cos(angle), y = std::sin(angle);
                draw->AddLine(ImVec2(center.x + x * dead_zone, center.y + y * dead_zone),
                              ImVec2(center.x + x * radius, center.y + y * radius),
                              hud_color(63, 166, 183, 180), 1.0f);
            }
            draw->AddCircle(center, radius, hud_color(72, 220, 235, 200), 64, 1.0f);
            draw->AddCircleFilled(center, dead_zone, hud_color(9, 24, 32, 245));
            draw->AddCircle(center, dead_zone, hud_color(72, 220, 235, 180), 48, 1.0f);
            draw->AddLine(ImVec2(center.x - 4, center.y), ImVec2(center.x + 4, center.y), hud_color(105, 235, 245), 1.0f);
            draw->AddLine(ImVec2(center.x, center.y - 4), ImVec2(center.x, center.y + 4), hud_color(105, 235, 245), 1.0f);
            if (IsKeyPressed(ImGuiKey_Escape)) state->SetBool(open_id, false);
            else if (IsMouseReleased(ImGuiMouseButton_Right)) {
                result = selected;
                state->SetBool(open_id, false);
            }
        }
        PopID();
        return result;
    }

    // 키 입력 설명 hud
    enum class HudInputType { Key, MouseLeft, MouseRight, MouseLeftHold, MouseRightHold };
    struct HudHint { const char* key; const char* description; HudInputType type; };
    void ControlsHud(const ImVec2& origin, const ImVec2& size, const HudHint* hints, int count)
    {
        if (!hints || count <= 0 || size.x <= 100.0f || size.y <= 50.0f) return;
                ImDrawList* draw = ImGui::GetWindowDrawList();
                const float padding = 12.0f, line = ImGui::GetTextLineHeight();
                const float row_height = line + 10.0f, header_height = line + 14.0f;
                float key_width = 0.0f, description_width = 0.0f;
                for (int i = 0; i < count; ++i) {
                    const auto& hint = hints[i];
                    key_width = std::max(key_width, ImGui::CalcTextSize(hint.key).x);
                    if (hint.type == HudInputType::MouseLeftHold || hint.type == HudInputType::MouseRightHold)
                        key_width = std::max(key_width, 18.0f + 2.0f + ImGui::CalcTextSize("꾸욲~").x * 0.7f);
                    description_width = std::max(description_width, ImGui::CalcTextSize(hint.description).x);
                }
                const float badge_width = key_width + 16.0f;
                const float width = std::min(size.x - 16.0f, std::max(185.0f, padding * 3 + badge_width + description_width));
                const float height = std::min(size.y - 16.0f, header_height + padding + row_height * count);
                const ImVec2 pos(origin.x + 8.0f, origin.y + size.y - height - 8.0f);
                const ImVec2 end(pos.x + width, pos.y + height);
                draw->PushClipRect(pos, end, true);
                const ImVec2 frame[] = {ImVec2(pos.x + 10, pos.y), ImVec2(end.x, pos.y),
                    ImVec2(end.x, end.y - 10), ImVec2(end.x - 10, end.y),
                    ImVec2(pos.x, end.y), ImVec2(pos.x, pos.y + 10)};
                draw->AddConvexPolyFilled(frame, 6, IM_COL32(17, 24, 28, 235));
                draw->AddPolyline(frame, 6, IM_COL32(246, 224, 70, 210), ImDrawFlags_Closed, 1.0f);
                draw->AddRectFilled(ImVec2(pos.x, pos.y + 11), ImVec2(pos.x + 3.0f, end.y - 5), IM_COL32(246, 224, 70, 240));
                draw->AddText(ImVec2(pos.x + padding, pos.y + 6.0f), IM_COL32(246, 224, 70, 255), "CONTROLS // INPUT");
                draw->AddLine(ImVec2(pos.x + padding, pos.y + header_height),
                              ImVec2(end.x - padding, pos.y + header_height), IM_COL32(246, 224, 70, 100));
                for (int i = 0; i < count; ++i) {
                    const float y = pos.y + header_height + 6.0f + i * row_height;
                    const ImVec2 badge_min(pos.x + padding, y);
                    const ImVec2 badge_max(badge_min.x + badge_width, y + line + 4.0f);
                    if (hints[i].type != HudInputType::Key) {
                        const bool hold = hints[i].type == HudInputType::MouseLeftHold || hints[i].type == HudInputType::MouseRightHold;
                        const float mouse_width = 18.0f;
                        const float group_width = mouse_width + (hold ? 2.0f + ImGui::CalcTextSize("꾸욲~").x * 0.7f : 0.0f);
                        const float mx = badge_min.x + (badge_width - group_width) * 0.5f;
                        const float my = y;
                        const float mouse_height = line + 4.0f;
                        draw->AddRectFilled(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height),
                                            IM_COL32(32, 39, 37, 255), 7.0f);
                        const bool left = hints[i].type == HudInputType::MouseLeft || hints[i].type == HudInputType::MouseLeftHold;
                        draw->AddRectFilled(ImVec2(mx + (left ? 1.0f : mouse_width * 0.5f), my + 1.0f),
                                            ImVec2(mx + (left ? mouse_width * 0.5f : mouse_width - 1.0f), my + mouse_height * 0.48f),
                                            IM_COL32(246, 224, 70, 255), 6.0f,
                                            left ? ImDrawFlags_RoundCornersTopLeft : ImDrawFlags_RoundCornersTopRight);
                        draw->AddRect(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height),
                                      IM_COL32(139, 148, 165, 255), 7.0f);
                        draw->AddLine(ImVec2(mx, my + mouse_height * 0.48f),
                                      ImVec2(mx + mouse_width, my + mouse_height * 0.48f), IM_COL32(139, 148, 165, 255));
                        draw->AddLine(ImVec2(mx + mouse_width * 0.5f, my + 1.0f),
                                      ImVec2(mx + mouse_width * 0.5f, my + mouse_height * 0.48f), IM_COL32(139, 148, 165, 255));
                        if (hold) {
                            const float font_size = ImGui::GetFontSize() * 0.7f;
                            draw->AddText(ImGui::GetFont(), font_size,
                                          ImVec2(mx + mouse_width + 2.0f, my),
                                          IM_COL32(255, 215, 80, 255), "꾸욲~");
                        }
                    } else {
                        draw->AddRectFilled(badge_min, badge_max, IM_COL32(32, 39, 37, 240));
                        draw->AddRect(badge_min, badge_max, IM_COL32(246, 224, 70, 190));
                        const float text_width = ImGui::CalcTextSize(hints[i].key).x;
                        draw->AddText(ImVec2(badge_min.x + (badge_width - text_width) * 0.5f, y + 2.0f),
                                      IM_COL32(225, 229, 237, 255), hints[i].key);
                    }
                    draw->AddText(ImVec2(badge_max.x + padding, y + 2.0f),
                                  IM_COL32(225, 229, 237, 255), hints[i].description);
                }
                draw->PopClipRect();
    }
}

int main(int argc, char **argv) {
    // ros2 초기화 & 노드 생성
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("imgui_camera_viewer");
    sensor_msgs::msg::Image::ConstSharedPtr latest_image_color;
    sensor_msgs::msg::Image::ConstSharedPtr latest_image_depth;
    std::mutex image_mutex;
    std::mutex depth_mutex;
    auto subscription = node->create_subscription<sensor_msgs::msg::Image>(
        "/camera/color/image_raw",                       // topic name
        rclcpp::SensorDataQoS(),                         // qos
        [&](sensor_msgs::msg::Image::ConstSharedPtr msg) // callback
        {
            std::lock_guard lock(image_mutex);
            latest_image_color = std::move(msg);
        }
    );
    auto subscription_depth = node->create_subscription<sensor_msgs::msg::Image>(
        "/camera/depth/image_raw",                       // topic name
        rclcpp::SensorDataQoS(),                         // qos
        [&](sensor_msgs::msg::Image::ConstSharedPtr msg) // callback
        {
            std::lock_guard lock(depth_mutex);
            latest_image_depth = std::move(msg);
        }
    );

    // ros2 스래드
    std::jthread ros_thread([&](std::stop_token stop)
    {
        rclcpp::executors::SingleThreadedExecutor executor;
        executor.add_node(node);
        while (!stop.stop_requested() && rclcpp::ok()) {
            executor.spin_once(std::chrono::milliseconds(50));
        }
    });


    // ImGui 초기화
    ImGui::Init("테스트 프로그램", 1280, 720);
    ImGui::LoadConfig("../imgui.ini");

    // 좌측 메뉴에 빈 도킹 영역을 추가한다.
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널0");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널1");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널2");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널3");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널4");







    ImGui::Image2D texture1 = ImGui::LoadImage(IMGUI_ROOT "/image.png");


    sensor_msgs::msg::Image::ConstSharedPtr image_msg;
    sensor_msgs::msg::Image::ConstSharedPtr depth_msg;
    while (rclcpp::ok() && !ImGui::ShouldClose()) {




    ImGui::Context([&]()
    {
        // 이미지 가져오기
        {
            std::lock_guard lock(image_mutex);
            if (latest_image_color) image_msg = std::move(latest_image_color);
        }
        {
             std::lock_guard lock(depth_mutex);
             if (latest_image_depth) depth_msg = std::move(latest_image_depth);
        }


        ImGui::Begin("Camera");
        static bool show_camera_selection = false;
        const ImVec2 camera_min = ImGui::GetCursorScreenPos();      // 윈도우의 좌상단 좌표
        const ImVec2 camera_size = ImGui::GetContentRegionAvail();  // 윈도우 크기
        if (image_msg) {
            if (ImGui::Image("camera",
                image_msg->data.data(),
                image_msg->data.size(),
                image_msg->width,
                image_msg->height,
                image_msg->step,
                ImGui::PixelFormat::RGB8,
                ImGui::GetContentRegionAvail()
            )) {
                const ImVec2 image_min = ImGui::GetItemRectMin();
                const ImVec2 image_max = ImGui::GetItemRectMax();
                const float width = image_max.x - image_min.x;
                const float height = image_max.y - image_min.y;
                // 임시 가상 카메라: 실제 영상에 정합하려면 카메라 보정값이 필요하다.
                const float near_plane = 0.1f, far_plane = 100.0f;
                const float f = 1.0f / std::tan(45.0f * 3.14159265f / 360.0f);
                Eigen::Matrix4f projection = Eigen::Matrix4f::Zero();
                projection(0, 0) = f * height / width;
                projection(1, 1) = f;
                projection(2, 2) = (far_plane + near_plane) / (near_plane - far_plane);
                projection(2, 3) = 2.0f * far_plane * near_plane / (near_plane - far_plane);
                projection(3, 2) = -1.0f;
                Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
                view(2, 3) = -3.0f;
                Eigen::Matrix4f transform = tf_control.cast<float>();
                ImGuizmo::BeginFrame();
                ImGuizmo::SetDrawlist();
                ImGuizmo::AllowAxisFlip(false);
                ImGuizmo::SetRect(image_min.x, image_min.y, width, height);
                if (!show_camera_selection) ImGuizmo::Manipulate(view.data(), projection.data(),
                                     ImGuizmo::TRANSLATE | ImGuizmo::ROTATE_X |
                                         ImGuizmo::ROTATE_Y | ImGuizmo::ROTATE_Z,
                                     ImGuizmo::LOCAL, transform.data());
                if (ImGuizmo::IsUsing()) tf_control = transform.cast<double>();
            }
        }

        const ImGui::HudHint camera_hints[] = {
            {"", "메뉴", ImGui::HudInputType::MouseRightHold},
            {"ESC", "메뉴 / 선택 닫기", ImGui::HudInputType::Key},
        };
        if (!show_camera_selection)
            ImGui::ControlsHud(camera_min, camera_size, camera_hints, IM_ARRAYSIZE(camera_hints));
        ShowTelemetryHud(camera_min, camera_size);

        // radial menu
        const char* camera_actions[] = {"선택하기", "집기", "놓기"};
        const int action = show_camera_selection ? -1 : ImGui::RadialMenu("CameraInteraction", camera_actions, 3);
        if (action == 0) ImGui::NotifyInfo("사이버펑크 스타일 노티피케이션");
        if (action == 1) show_camera_selection = true;



        ImGui::CameraSelectionPanel(show_camera_selection, camera_min, camera_size);
        ImGui::End();

        ImGui::Begin("Depth");

        if (depth_msg) {
            ImGui::Image("depth",
                depth_msg->data.data(),
                depth_msg->data.size(),
                depth_msg->width,
                depth_msg->height,
                depth_msg->step,
                ImGui::PixelFormat::Depth32F,
                ImGui::GetContentRegionAvail()
            );
        }
        ImGui::End();



        ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

        if (ImGui::BeginCollapsingHeader(ICON_MD_WIDGETS " Widgets Example ")) {
            ImGui::BeginChild("Widgets_Child", ImVec2(0, 1150), true);

            // ==========================================
            // 1. 기본 컨트롤 (Basic Controls)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_CHECK_BOX " Basic Controls");
            ImGui::Separator();

            static bool is_c = false;
            if (ImGui::Check("체크박스", &is_c)) {
                if (is_c)
                    ImGui::NotifyInfo("체크박스 선택됨");
                else
                    ImGui::NotifyInfo("체크박스 해제됨");
            }

            static int radio_idx = 0;
            if (ImGui::Radio("라디오 1", &radio_idx, 0))
                ImGui::NotifyInfo("라디오 1 선택됨");
            if (ImGui::Radio("라디오 2", &radio_idx, 1))
                ImGui::NotifyInfo("라디오 2 선택됨");

            static bool is_t = false;
            if (ImGui::Toggle("토글", &is_t)) {
                if (is_t)
                    ImGui::NotifyInfo("토글 선택됨");
                else
                    ImGui::NotifyInfo("토글 해제됨");
            }

            float btn_width = ImGui::GetContentRegionAvail().x / 4.15f;
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(100, 141, 222, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(120, 161, 242, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 80, 121, 202, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n정보", ImVec2(btn_width, 0))) {
                ImGui::NotifyInfo("정보 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32( 46, 160,  67, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32( 66, 180,  87, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 26, 140,  47, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n성공", ImVec2(btn_width, 0))) {
                ImGui::NotifySucc("성공 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(227, 179,  65, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(247, 199,  85, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(207, 159,  45, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n경고", ImVec2(btn_width, 0))) {
                ImGui::NotifyWarn("경고 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(224, 108, 117, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(244, 128, 137, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(204,  88,  97, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n에러", ImVec2(btn_width, 0))) {
                ImGui::NotifyError("에러 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 2. 텍스트 입력 (Text Inputs)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_EDIT " Text Inputs");
            ImGui::Separator();

            static char text_buf[128] = "Hello, Custom ImGui!";
            ImGui::InputTextLine("텍스트 입력", text_buf, IM_COUNTOF(text_buf));

            static char pw_buf[64] = "";
            ImGui::InputTextLine("비밀번호", pw_buf, IM_COUNTOF(pw_buf), ImGuiInputTextFlags_Password);

            static char multiline_buf[256] = "여기에\n여러 줄의\n텍스트를 입력하세요.";
            ImGui::InputTextLines("메모", multiline_buf, IM_COUNTOF(multiline_buf), ImVec2(0, 80));
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 3. 슬라이더 및 드래그 (Sliders & Drags)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TUNE " Sliders & Drags");
            ImGui::Separator();

            static double drag_value = 50.0f;
            ImGui::Drag("Drag", &drag_value, 1.0f, 0.0f, 100.0f, "%.1f");

            static float slider_value = 30.0f;
            ImGui::Slider("Slider2", &slider_value, 0.0f, 100.0f);

            static float range_min = 20.0f, range_max = 80.0f;
            ImGui::SliderRange("Slider Range2", &range_min, &range_max, 0.0f, 100.0f);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // Input Number
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_NUMBERS " Input Number");
            ImGui::Separator();

            static int input_scalar_value = 50;
            ImGui::InputNum("Input Int", &input_scalar_value);

            static double input_double_value = 50.0;
            ImGui::InputNum("Input Double", &input_double_value, 0.01, 0, "%0.2f");
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
            ImGui::Separator();

            static int current_theme = 1;
            ImGui::DropDown("DropDown", &current_theme, "선택 1\0선택 2\0선택 3\0", 3);

            static int listbox_item_current = 1;
            const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange" };
            ImGui::List("과일 목록", &listbox_item_current, listbox_items, IM_COUNTOF(listbox_items), 4);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 5. 색상 선택기 (Color Pickers)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_COLOR_LENS " Color Pickers");
            ImGui::Separator();

            static float color_rgb[3] = { 0.36f, 0.41f, 0.94f };
            ImGui::ColorEdit3X("컬러3", color_rgb);

            static float color_rgba[4] = { 0.8f, 0.2f, 0.3f, 0.5f };
            ImGui::ColorEdit4X("컬러4", color_rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);

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
            ImGui::BeginChild("상태바", ImVec2(0, 230), true);

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
            if (ImGui::InputNum("##상태", &current_account_status)) {
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

                if (texture1 && texture1->id != 0) {
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

            }
            ImGui::EndChild();
            ImGui::EndCollapsingHeader();

        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_COLORS " Theme Example ")) {

            static int current_theme = 1;

            ImGui::BeginChild("ThemeSelector", ImVec2(0, 150), true);
            if (ImGui::ThemeSelector(&ImGui::theme_id))
            {
                // 테마가 변경되었을 때 실행할 로직
                if (ImGui::theme_id == 0) {
                    ImGui::ThemeWhite();
                }
                else {
                    ImGui::ThemeDark();
                }

            }
            ImGui::EndChild();

            ImGui::EndCollapsingHeader();
        }

        ImGui::End();
    });  }



    ImGui::Destroy();
    ros_thread.request_stop();
    ros_thread.join();
    rclcpp::shutdown();
    return 0;
}


