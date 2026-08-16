#include "ui_stepbar.h"
#include <cstdint>

bool ImGui::StatusStepBar(const char *str_id, int *current_step, const char **step_labels, int num_steps)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(str_id);

    float circle_radius = 12.0f; // 원의 반지름
    float line_thickness = 4.0f; // 연결 선의 두께
    float line_gap = 6.0f;       // 원과 선 사이의 간격

    // 텍스트를 위한 대략적인 높이 계산 (2줄까지 지원)
    float text_height = g.FontSize * 2.5f;
    float content_height = circle_radius * 2.0f + 10.0f + text_height; // 위젯이 실제로 차지하는 높이

    // 가용 공간(현재 렌더링 가능한 영역) 가져오기
    ImVec2 avail = GetContentRegionAvail();

    // 1. 너무 넓게 퍼지지 않도록 '최대 너비' 설정 (노드 1개당 약 120px)
    float max_width = num_steps * 120.0f;
    float content_width = (avail.x < max_width) ? avail.x : max_width;

    // 2. 가로/세로 가운데 정렬 오프셋 계산
    float offset_x = (avail.x > content_width) ? (avail.x - content_width) * 0.5f : 0.0f;

    // Child 창처럼 세로 공간이 고정되어 남을 경우 수직 중앙 정렬도 수행
    float offset_y = (avail.y > content_height) ? (avail.y - content_height) * 0.5f : 0.0f;
    window->DC.CursorPos.y += offset_y; // 커서를 아래로 밀어서 수직 중앙 정렬 적용

    // 전체 가용 너비(avail.x)를 점유하도록 Bounding Box 설정 (다음 UI가 겹치지 않게)
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(avail.x, content_height));
    ItemSize(frame_bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    // 양끝 여백(20px)을 고려하여 간격 계산 및 렌더링 시작 좌표에 가로 오프셋(offset_x) 적용
    float usable_width = content_width - (circle_radius * 2.0f) - 40.0f;
    float start_x = frame_bb.Min.x + offset_x + 20.0f + circle_radius;
    float start_y = frame_bb.Min.y + circle_radius + 5.0f;
    float step_spacing = usable_width / (float)(num_steps - 1);

    bool value_changed = false;
    ImDrawList* draw_list = window->DrawList;

    // 색상 정의
    ImU32 active_col = IM_COL32(98, 192, 115, 255);         // 부드러운 그린 (활성)
    ImU32 inactive_col = IM_COL32(43, 45, 49, 255);         // 다크 그레이 (비활성)
    ImU32 text_active_col = IM_COL32(240, 240, 240, 255);   // 밝은 텍스트 (활성)
    ImU32 text_inactive_col = IM_COL32(130, 130, 130, 255); // 어두운 텍스트 (비활성)
    ImU32 icon_col = IM_COL32(20, 20, 20, 255);             // 체크 아이콘

    // 1. 배경 연결 선 그리기
    for (int i = 0; i < num_steps - 1; ++i)
    {
        ImVec2 p1 = ImVec2(start_x + i * step_spacing + circle_radius + line_gap, start_y);
        ImVec2 p2 = ImVec2(start_x + (i + 1) * step_spacing - circle_radius - line_gap, start_y);

        // 현재 단계 이전의 선은 활성 색상, 이후는 비활성 색상 적용
        ImU32 current_line_col = (*current_step > i) ? active_col : inactive_col;
        float half_thickness = line_thickness * 0.5f;

        draw_list->AddRectFilled(
            ImVec2(p1.x, p1.y - half_thickness),
            ImVec2(p2.x, p2.y + half_thickness),
            current_line_col,
            half_thickness
        );
    }

    // 2. 원, 아이콘, 텍스트 렌더링 및 상호작용
    for (int i = 0; i < num_steps; ++i)
    {
        ImVec2 center = ImVec2(start_x + i * step_spacing, start_y);
        bool is_active = (i <= *current_step); // 현재 단계이거나 이전 단계면 활성화

        // 각 원(Step)의 클릭 영역
        ImRect circle_bb(
            ImVec2(center.x - circle_radius - 10.0f, center.y - circle_radius - 10.0f),
            ImVec2(center.x + circle_radius + 10.0f, center.y + circle_radius + 10.0f)
        );

        bool hovered, held;
        ImGuiID step_id = window->GetID((void*)(intptr_t)i);
        bool pressed = ButtonBehavior(circle_bb, step_id, &hovered, &held);

        if (pressed && *current_step != i)
        {
            *current_step = i;
            value_changed = true;
            MarkItemEdited(id);
        }

        // 호버 효과 및 활성/비활성 색상 처리
        ImU32 current_circle_col = is_active ? active_col : inactive_col;
        if (hovered && !is_active)
        {
            current_circle_col = IM_COL32(65, 67, 74, 255);
        }

        // 원 그리기
        draw_list->AddCircleFilled(center, circle_radius, current_circle_col);

        // 활성화된 단계에 체크 아이콘 그리기
        if (is_active)
        {
            float check_thickness = 2.5f;
            ImVec2 p1 = ImVec2(center.x - 5.0f, center.y + 1.0f);
            ImVec2 p2 = ImVec2(center.x - 1.5f, center.y + 4.5f);
            ImVec2 p3 = ImVec2(center.x + 5.5f, center.y - 3.5f);

            draw_list->PathLineTo(p1);
            draw_list->PathLineTo(p2);
            draw_list->PathLineTo(p3);
            draw_list->PathStroke(icon_col, 0, check_thickness);
        }

        // 하단 텍스트 렌더링
        if (step_labels && step_labels[i])
        {
            const char* text = step_labels[i];
            ImVec2 text_pos = ImVec2(center.x, center.y + circle_radius + 12.0f);

            ImU32 current_text_col = is_active ? text_active_col : text_inactive_col;

            const char* line_start = text;
            const char* line_end = strchr(line_start, '\n');
            while (line_start != NULL)
            {
                if (!line_end) line_end = line_start + strlen(line_start);
                ImVec2 line_size = CalcTextSize(line_start, line_end);

                draw_list->AddText(ImVec2(center.x - line_size.x * 0.5f, text_pos.y), current_text_col, line_start, line_end);

                text_pos.y += g.FontSize + 2.0f;
                if (*line_end == '\0') break;

                line_start = line_end + 1;
                line_end = strchr(line_start, '\n');
            }
        }
    }

    return value_changed;
}
