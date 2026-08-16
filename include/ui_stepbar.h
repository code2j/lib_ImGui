#pragma once
#include "imgui.h"
#include "imgui_internal.h"


namespace ImGui
{
    /*
    // [사용 예시]
        const char* status_labels[] = {
            "시스템 시작",
            "시스템 초기화",
            "시스템 준비",
            "시스템 동작",
        };

        static int current_account_status = 2;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0);
        ImGui::BeginChild("상태바", ImVec2(0, 150), true);
        if (ImGui::InputInt("상태", &current_account_status)) {
            current_account_status = std::max(0, std::min(current_account_status, 3));
        }
        ImGui::StatusStepBar("##AccountStatusStepBar", &current_account_status, status_labels, 4);
        ImGui::EndChild();
        ImGui::PopStyleVar();
    */
    bool StatusStepBar(const char* str_id, int* current_step, const char** step_labels, int num_steps);
}
