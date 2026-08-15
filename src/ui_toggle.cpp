#include "ui_toggle.h"
#include "imgui_internal.h"


namespace ImGui
{

    bool ToggleButton(const char* str_id, bool* v)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight() * 0.6f;
        float width = height * 1.99f;
        float radius = height * 0.50f;

        bool turned_on = false; // 켜졌는지 여부 기록

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if (ImGui::IsItemClicked())
        {
            *v = !*v;
            // 클릭되었고, 반전된 상태가 true(켜짐)일 때만 true 저장
            if (*v)
            {
                turned_on = true;
            }
        }

        float t = *v ? 1.0f : 0.0f;

        ImGuiContext& g = *GImGui;
        float ANIM_SPEED = 0.08f;
        if (g.LastActiveId == g.CurrentWindow->GetID(str_id))
        {
            float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
            t = *v ? (t_anim) : (1.0f - t_anim);
        }

        ImU32 col_bg;
        if (ImGui::IsItemHovered())
        {
            // 마우스 오버 시 약간 더 밝은 색상 적용
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(31.0f / 255.0f, 31.0f / 255.0f, 32.0f / 255.0f, 1.0f),   // Hovered OFF
                ImVec4(113.0f / 255.0f, 125.0f / 255.0f, 255.0f / 255.0f, 1.0f), // Hovered ON
                t));
        }
        else
        {
            // 요청하신 기본 색상 적용
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(11.0f / 255.0f, 11.0f / 255.0f, 12.0f / 255.0f, 1.0f),   // Normal OFF (11, 11, 12)
                ImVec4(93.0f / 255.0f, 105.0f / 255.0f, 240.0f / 255.0f, 1.0f), // Normal ON (93, 105, 240)
                t));
        }

        // 배경(채우기) 그리기
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);

        // 테두리(선) 그리기 추가
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border); // 테두리 색상 (필요시 ImColor(255, 255, 255, 100) 등으로 변경 가능)
        float border_thickness = 1.5f; // 테두리 두께
        draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), col_border, height * 0.5f, 0, border_thickness);

        // 안쪽 원형 토글 그리기
        draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));

        return turned_on; // OFF->ON으로 켜지는 순간에만 true 반환
    }
}
