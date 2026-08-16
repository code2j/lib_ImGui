#pragma once
#include "ui_widgets.h"
#include "imgui_internal.h"
#include <cstdint>


bool ImGui::BeginCollapsingHeader(const char* label, bool default_open)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiID id = window->GetID(label);

    // 상태 및 애니메이션 변수 가져오기
    bool is_open = window->StateStorage.GetInt(id, default_open ? 1 : 0);
    float anim_t = window->StateStorage.GetFloat(id + 1, is_open ? 1.0f : 0.0f);
    float max_height = window->StateStorage.GetFloat(id + 2, 0.0f);

    bool calculating_height = (is_open && max_height == 0.0f);

    // =========================================================================
    // 헤더 UI 영역 계산
    // =========================================================================
    ImVec2 pos = window->DC.CursorPos;
    float frame_height = ImGui::GetFrameHeight();

    ImRect bb;
    bb.Min.x = window->WorkRect.Min.x;
    bb.Max.x = window->WorkRect.Max.x;
    bb.Min.y = pos.y;
    bb.Max.y = pos.y + frame_height;

    const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f);
    bb.Min.x -= outer_extend;
    bb.Max.x += outer_extend;

    // 아이템 크기 등록
    ImGui::ItemSize(ImVec2(window->WorkRect.Max.x - pos.x, frame_height));

    bool is_visible = ImGui::ItemAdd(bb, id);

    if (is_visible) {
        // 클릭 상호작용 처리
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed) {
            is_open = !is_open;
            window->StateStorage.SetInt(id, is_open ? 1 : 0);
        }

        // 배경 렌더링
        ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        ImGui::RenderFrame(bb.Min, bb.Max, bg_col, false, ImGui::GetStyle().FrameRounding);

        ImVec2 padding = ImGui::GetStyle().FramePadding;
        ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

        // =========================================================================
        // [수정] 우측 끝 V자 화살표(Chevron) 방향 변경 (닫힘: 오른쪽, 열림: 아래)
        // =========================================================================
        ImVec2 chevron_center = ImVec2(bb.Max.x - padding.x - g.FontSize * 0.5f, bb.Min.y + frame_height * 0.5f);
        float chevron_size = 5.0f; // 화살표 크기 조절
        ImVec2 p1, p2, p3;

        if (is_open) {
            // 열림 (아래쪽 향함: v)
            p1 = ImVec2(chevron_center.x - chevron_size, chevron_center.y - chevron_size * 0.4f);
            p2 = ImVec2(chevron_center.x, chevron_center.y + chevron_size * 0.6f);
            p3 = ImVec2(chevron_center.x + chevron_size, chevron_center.y - chevron_size * 0.4f);
        } else {
            // 닫힘 (오른쪽 향함: >)
            p1 = ImVec2(chevron_center.x - chevron_size * 0.4f, chevron_center.y - chevron_size);
            p2 = ImVec2(chevron_center.x + chevron_size * 0.6f, chevron_center.y);
            p3 = ImVec2(chevron_center.x - chevron_size * 0.4f, chevron_center.y + chevron_size);
        }

        ImVec2 points[3] = { p1, p2, p3 };
        window->DrawList->AddPolyline(points, 3, text_col, 0, 2.0f);

        // 텍스트 좌측 정렬 및 클리핑 렌더링
        ImVec2 text_pos(window->WorkRect.Min.x + padding.x, bb.Min.y + padding.y);
        ImVec2 clip_rect_max = ImVec2(bb.Max.x - padding.x * 2.0f - g.FontSize, bb.Max.y);
        ImGui::RenderTextClipped(text_pos, clip_rect_max, label, NULL, NULL);
    }

    // =========================================================================

    // 애니메이션 프레임 업데이트
    if (!calculating_height) {
        float speed = ImGui::GetIO().DeltaTime * 7.0f;
        anim_t = ImClamp(anim_t + (is_open ? speed : -speed), 0.0f, 1.0f);
        window->StateStorage.SetFloat(id + 1, anim_t);
    }

    // 내용 영역 (Child Window) 열기
    if (anim_t > 0.0f || calculating_height) {
        float current_height = calculating_height ? 0.0f : (max_height * anim_t);
        float alpha = calculating_height ? 0.0f : anim_t;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().ChildRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGuiChildFlags child_flags = 0;

        if (calculating_height || (is_open && anim_t >= 1.0f)) {
            child_flags |= ImGuiChildFlags_AutoResizeY;
            current_height = 0.0f;
        }

        ImGui::BeginChild(id + 3, ImVec2(0, current_height), child_flags, window_flags);
        return true;
    }

    return false;
}

void ImGui::EndCollapsingHeader(const char* label)
{
    ImGuiWindow* child_window = ImGui::GetCurrentWindow();

    float height = child_window->DC.CursorMaxPos.y - child_window->Pos.y + ImGui::GetStyle().WindowPadding.y;

    ImGui::EndChild();
    ImGui::PopStyleVar(3);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID(label);

    bool is_open = window->StateStorage.GetInt(id, 0);
    float anim_t = window->StateStorage.GetFloat(id + 1, 0.0f);
    float max_height = window->StateStorage.GetFloat(id + 2, 0.0f);

    if (is_open && (max_height == 0.0f || anim_t >= 1.0f)) {
        window->StateStorage.SetFloat(id + 2, height);
    }
}

bool ImGui::SliderFloatRange(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = ImGui::CalcItemWidth();

    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
        return false;

    float grab_width = 12.0f;
    float track_height = 4.0f;

    float track_x0 = frame_bb.Min.x + grab_width * 0.5f;
    float track_x1 = frame_bb.Max.x - grab_width * 0.5f;
    float track_w = track_x1 - track_x0;

    auto ValToPos = [&](float v) -> float {
        float t = (v - v_bound_min) / (v_bound_max - v_bound_min);
        t = ImClamp(t, 0.0f, 1.0f);
        return track_x0 + t * track_w;
    };

    auto PosToVal = [&](float x) -> float {
        float t = ImClamp((x - track_x0) / track_w, 0.0f, 1.0f);
        return v_bound_min + t * (v_bound_max - v_bound_min);
    };

    float left_x = ValToPos(*v_min);
    float right_x = ValToPos(*v_max);

    bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool value_changed = false;

    ImGuiStorage* storage = window->DC.StateStorage;
    int active_grab = storage->GetInt(id, 0);

    // 클릭한 위치와 그랩 중심 사이의 거리(Offset)를 저장할 ID
    ImGuiID offset_id = id + 1;

    float hitbox_radius = grab_width * 0.5f + 4.0f;
    ImRect left_grab_bb(ImVec2(left_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(left_x + hitbox_radius, frame_bb.Max.y + 4.0f));
    ImRect right_grab_bb(ImVec2(right_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(right_x + hitbox_radius, frame_bb.Max.y + 4.0f));

    // ==========================================
    // 클릭 이벤트 처리 로직
    // ==========================================
    if (hovered && g.IO.MouseClicked[0])
    {
        bool left_hovered = left_grab_bb.Contains(g.IO.MousePos);
        bool right_hovered = right_grab_bb.Contains(g.IO.MousePos);

        if (left_hovered || right_hovered)
        {
            if (left_hovered && right_hovered) {
                float dist_l = std::abs(g.IO.MousePos.x - left_x);
                float dist_r = std::abs(g.IO.MousePos.x - right_x);
                active_grab = (dist_l <= dist_r) ? 1 : 2;
            } else if (left_hovered) {
                active_grab = 1;
            } else {
                active_grab = 2;
            }

            storage->SetInt(id, active_grab);

            // [추가됨] 마우스를 클릭한 시점에, 그랩의 중심점과 마우스의 X 좌표 차이를 계산해서 저장
            float grab_center_x = (active_grab == 1) ? left_x : right_x;
            float click_offset = g.IO.MousePos.x - grab_center_x;
            storage->SetFloat(offset_id, click_offset);

            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }
    }

    // ==========================================
    // 드래그 중인 상태 처리
    // ==========================================
    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            // [추가/수정됨] 마우스 좌표에서 아까 저장해둔 Offset을 빼서 부드럽게 조작되도록 보정
            float click_offset = storage->GetFloat(offset_id, 0.0f);
            float adjusted_mouse_x = g.IO.MousePos.x - click_offset;

            float v_new = PosToVal(adjusted_mouse_x);
            if (active_grab == 1)
            {
                *v_min = ImMin(v_new, *v_max);
                value_changed = true;
            }
            else if (active_grab == 2)
            {
                *v_max = ImMax(v_new, *v_min);
                value_changed = true;
            }

            left_x = ValToPos(*v_min);
            right_x = ValToPos(*v_max);
        }
        else
        {
            ImGui::ClearActiveID();
            storage->SetInt(id, 0);
            active_grab = 0;
        }
    }

    if (value_changed)
        ImGui::MarkItemEdited(id);

    // ==========================================
    // --- 커스텀 렌더링 시작 ---
    // ==========================================
    ImGui::RenderNavCursor(frame_bb, id);

    float track_y = std::floor(frame_bb.GetCenter().y + 0.5f);
    float lx = std::floor(left_x + 0.5f);
    float rx = std::floor(right_x + 0.5f);

    ImVec2 track_min = ImVec2(frame_bb.Min.x, track_y - track_height * 0.5f);
    ImVec2 track_max = ImVec2(frame_bb.Max.x, track_y + track_height * 0.5f);

    ImU32 bg_track_col = IM_COL32(65, 65, 70, 255);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    window->DrawList->AddRectFilled(
        ImVec2(lx, track_min.y),
        ImVec2(rx, track_max.y),
        fill_track_col, track_height * 0.5f);

    ImU32 grab_col = IM_COL32(255, 255, 255, 255);
    float tri_w = 7.0f;
    float tri_h = 9.0f;

    auto AddRoundedTriangleFilled = [](ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImVec2 p3, float radius, ImU32 col)
    {
        ImVec2 pts[3] = { p1, p2, p3 };

        for (int i = 0; i < 3; i++)
        {
            ImVec2 prev = pts[(i + 2) % 3];
            ImVec2 curr = pts[i];
            ImVec2 next = pts[(i + 1) % 3];

            // 변 벡터 계산
            ImVec2 v1 = ImVec2(prev.x - curr.x, prev.y - curr.y);
            ImVec2 v2 = ImVec2(next.x - curr.x, next.y - curr.y);

            float len1 = std::sqrt(v1.x * v1.x + v1.y * v1.y);
            float len2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
            if (len1 > 0.0f) { v1.x /= len1; v1.y /= len1; }
            if (len2 > 0.0f) { v2.x /= len2; v2.y /= len2; }

            float angle1 = std::atan2(v1.y, v1.x);
            float angle2 = std::atan2(v2.y, v2.x);

            // [핵심] 호를 항상 렌더링 내부 방향(짧은 쪽)으로 그리도록 각도 정규화
            float diff = angle2 - angle1;
            while (diff < -IM_PI) diff += IM_PI * 2.0f;
            while (diff > IM_PI) diff -= IM_PI * 2.0f;

            angle2 = angle1 + diff;

            draw_list->PathArcTo(curr, radius, angle1, angle2, 6);
        }

        draw_list->PathFillConvex(col);
    };

    float corner_radius = 2.0f; // 둥근 모서리 반지름 (1.5f ~ 2.5f 추천)

    // 왼쪽 그랩 (▶ 둥근 모양)
    AddRoundedTriangleFilled(
        window->DrawList,
        ImVec2(lx - tri_w, track_y - tri_h),
        ImVec2(lx + tri_w, track_y),
        ImVec2(lx - tri_w, track_y + tri_h),
        corner_radius,
        grab_col
    );

    // 오른쪽 그랩 (◀ 둥근 모양)
    AddRoundedTriangleFilled(
        window->DrawList,
        ImVec2(rx + tri_w, track_y - tri_h),
        ImVec2(rx + tri_w, track_y + tri_h),
        ImVec2(rx - tri_w, track_y),
        corner_radius,
        grab_col
    );

    // 툴팁 표시
    if (g.ActiveId == id && active_grab != 0)
    {
        char value_buf[64];
        float active_val = (active_grab == 1) ? *v_min : *v_max;
        snprintf(value_buf, sizeof(value_buf), format, active_val);

        ImVec2 text_size = ImGui::CalcTextSize(value_buf);
        ImVec2 padding(10.0f, 6.0f);
        float tooltip_y_offset = 12.0f;
        float grab_top_offset = tri_h;

        float active_x = (active_grab == 1) ? left_x : right_x;
        ImVec2 tooltip_min = ImVec2(active_x - text_size.x * 0.5f - padding.x, track_y - grab_top_offset - tooltip_y_offset - text_size.y - padding.y * 2.0f);
        ImVec2 tooltip_max = ImVec2(active_x + text_size.x * 0.5f + padding.x, track_y - grab_top_offset - tooltip_y_offset);

        ImU32 tooltip_bg_col = IM_COL32(25, 25, 28, 255);
        ImU32 tooltip_border_col = IM_COL32(55, 55, 60, 255);

        window->DrawList->AddRectFilled(tooltip_min, tooltip_max, tooltip_bg_col, 6.0f);
        window->DrawList->AddRect(tooltip_min, tooltip_max, tooltip_border_col, 6.0f);

        ImVec2 p1 = ImVec2(active_x - 6.0f, tooltip_max.y);
        ImVec2 p2 = ImVec2(active_x + 6.0f, tooltip_max.y);
        ImVec2 p3 = ImVec2(active_x, tooltip_max.y + 5.0f);
        window->DrawList->AddTriangleFilled(p1, p2, p3, tooltip_bg_col);

        window->DrawList->AddText(tooltip_min + padding, IM_COL32(230, 230, 230, 255), value_buf);
    }
    // ==========================================
    // --- 커스텀 렌더링 끝 ---
    // ==========================================

    if (label_size.x > 0.0f)
        ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}

bool ImGui::ToggleButton(const char* str_id, bool* v)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight() * 0.6f;
    float width = height * 1.99f;

    float outer_radius = height * 0.50f;
    float inner_radius = outer_radius * 0.7f;

    bool turned_on = false;

    // InvisibleButton으로 위젯 영역 등록 및 상호작용 처리
    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
    {
        *v = !*v;
        if (*v)
        {
            turned_on = true;
        }
    }

    // =========================================================================
    // [핵심 수정] StateStorage 기반의 매끄러운 부드러운 애니메이션 (Lerp)
    // =========================================================================
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID(str_id);

    // 저장된 애니메이션 상태(0.0f ~ 1.0f) 가져오기
    float anim_t = window->StateStorage.GetFloat(id, *v ? 1.0f : 0.0f);

    // 목표 값 설정 (ON이면 1.0f, OFF면 0.0f)
    float target_t = *v ? 1.0f : 0.0f;

    // 프레임 독립적인 지수 감쇠 보간 (숫자가 클수록 애니메이션 속도가 빨라집니다)
    // 15.0f ~ 20.0f 정도가 가장 기분 좋은 쫀득한 속도를 제공합니다.
    float speed = 15.0f;
    anim_t = ImLerp(anim_t, target_t, ImGui::GetIO().DeltaTime * speed);

    // 미세한 오차 보정 (0 또는 1에 매우 가까워지면 고정)
    if (ImAbs(anim_t - target_t) < 0.0001f)
        anim_t = target_t;

    // 업데이트된 애니메이션 상태 저장
    window->StateStorage.SetFloat(id, anim_t);

    // =========================================================================
    // 렌더링 (t 대신 부드러운 anim_t 사용)
    // =========================================================================
    ImU32 col_bg;
    if (ImGui::IsItemHovered())
    {
        col_bg = ImGui::GetColorU32(ImLerp(
            ImVec4(31.0f / 255.0f, 31.0f / 255.0f, 32.0f / 255.0f, 1.0f),   // Hovered OFF
            ImVec4(113.0f / 255.0f, 125.0f / 255.0f, 255.0f / 255.0f, 1.0f), // Hovered ON
            anim_t));
    }
    else
    {
        col_bg = ImGui::GetColorU32(ImLerp(
            ImVec4(11.0f / 255.0f, 11.0f / 255.0f, 12.0f / 255.0f, 1.0f),   // Normal OFF
            ImVec4(93.0f / 255.0f, 105.0f / 255.0f, 240.0f / 255.0f, 1.0f), // Normal ON
            anim_t));
    }

    // 배경(채우기) 그리기
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);

    // 테두리(선) 그리기
    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    float border_thickness = 1.5f;
    draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), col_border, height * 0.5f, 0, border_thickness);

    // 안쪽 원형 토글 그리기 (anim_t를 활용해 원 위치 보간)
    ImVec2 circle_center = ImVec2(p.x + outer_radius + anim_t * (width - outer_radius * 2.0f), p.y + outer_radius);
    draw_list->AddCircleFilled(circle_center, inner_radius, IM_COL32(255, 255, 255, 255));

    return turned_on;
}

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

    // 텍스트 높이 계산
    float text_height = g.FontSize * 2.5f;
    float content_height = circle_radius * 2.0f + 10.0f + text_height;

    // 가용 공간 가져오기
    ImVec2 avail = GetContentRegionAvail();

    // 1. 최대 너비 설정
    float max_width = num_steps * 120.0f;
    float content_width = (avail.x < max_width) ? avail.x : max_width;

    // 2. 오프셋 계산
    float offset_x = (avail.x > content_width) ? (avail.x - content_width) * 0.5f : 0.0f;
    float offset_y = (avail.y > content_height) ? (avail.y - content_height) * 0.5f : 0.0f;
    window->DC.CursorPos.y += offset_y;

    // Bounding Box 설정
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(avail.x, content_height));
    ItemSize(frame_bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    // 배치 좌표 계산
    float usable_width = content_width - (circle_radius * 2.0f) - 40.0f;
    float start_x = frame_bb.Min.x + offset_x + 20.0f + circle_radius;
    float start_y = frame_bb.Min.y + circle_radius + 5.0f;
    float step_spacing = usable_width / (float)(num_steps - 1);

    bool value_changed = false;
    ImDrawList* draw_list = window->DrawList;

    // 색상 정의
    ImVec4 active_col_v = ImVec4(98.0f/255.0f, 192.0f/255.0f, 115.0f/255.0f, 1.0f);   // 부드러운 그린 (활성)
    ImVec4 inactive_col_v = ImVec4(43.0f/255.0f, 45.0f/255.0f, 49.0f/255.0f, 1.0f);   // 다크 그레이 (비활성)
    ImVec4 hover_col_v = ImVec4(65.0f/255.0f, 67.0f/255.0f, 74.0f/255.0f, 1.0f);       // 호버 색상

    ImU32 text_active_col = IM_COL32(240, 240, 240, 255);
    ImU32 text_inactive_col = IM_COL32(130, 130, 130, 255);
    ImU32 icon_col = IM_COL32(20, 20, 20, 255);

    float delta_time = GetIO().DeltaTime;
    float anim_speed = 10.0f; // 애니메이션 속도

    // =========================================================================
    // 1. 각 단계별 애니메이션 상태(0.0 ~ 1.0) 업데이트
    // =========================================================================
    // 각 스텝의 애니메이션 값을 임시 보관할 배열
    static ImVector<float> step_anims;
    step_anims.resize(num_steps);

    for (int i = 0; i < num_steps; ++i)
    {
        ImGuiID node_id = window->GetID((void*)(intptr_t)(i + 1000));
        float target_t = (i <= *current_step) ? 1.0f : 0.0f;
        float current_t = window->StateStorage.GetFloat(node_id, target_t);

        // Lerp 보간
        current_t = ImLerp(current_t, target_t, delta_time * anim_speed);
        if (ImAbs(current_t - target_t) < 0.001f)
            current_t = target_t;

        window->StateStorage.SetFloat(node_id, current_t);
        step_anims[i] = current_t;
    }

    // =========================================================================
    // 2. 연결 선(Line) 애니메이션 렌더링
    // =========================================================================
    for (int i = 0; i < num_steps - 1; ++i)
    {
        ImVec2 p1 = ImVec2(start_x + i * step_spacing + circle_radius + line_gap, start_y);
        ImVec2 p2 = ImVec2(start_x + (i + 1) * step_spacing - circle_radius - line_gap, start_y);
        float half_thickness = line_thickness * 0.5f;

        // 비활성 배경 선 그리기
        draw_list->AddRectFilled(
            ImVec2(p1.x, p1.y - half_thickness),
            ImVec2(p2.x, p2.y + half_thickness),
            GetColorU32(inactive_col_v),
            half_thickness
        );

        // 다음 노드의 애니메이션 값(step_anims[i+1])을 기준으로 선이 차오르는 게이지 효과
        float line_progress = step_anims[i + 1];

        // 미세한 연산 오차로 p1.x 근처에서 라운딩 사각형이 남아 보이는 현상을 방지합니다.
        if (line_progress > 0.01f)
        {
            float current_x = ImLerp(p1.x, p2.x, line_progress);

            // p1.x보다 오버슛(도넘침)하여 넘어가지 않도록 클램핑 및 유효 범위 체크
            if (current_x > p1.x + 0.5f)
            {
                ImVec2 active_p2 = ImVec2(current_x, p2.y);
                draw_list->AddRectFilled(
                    ImVec2(p1.x, p1.y - half_thickness),
                    ImVec2(active_p2.x, active_p2.y + half_thickness),
                    GetColorU32(active_col_v),
                    half_thickness
                );
            }
        }
    }

    // =========================================================================
    // 3. 원, 아이콘, 텍스트 애니메이션 렌더링 및 상호작용
    // =========================================================================
    for (int i = 0; i < num_steps; ++i)
    {
        ImVec2 center = ImVec2(start_x + i * step_spacing, start_y);
        float anim_t = step_anims[i]; // 0.0(비활성) ~ 1.0(활성)

        // 클릭 영역
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

        // 호버 애니메이션 처리 (StateStorage)
        ImGuiID hover_id = window->GetID((void*)(intptr_t)(i + 2000));
        float hover_target = (hovered && anim_t < 0.5f) ? 1.0f : 0.0f;
        float hover_t = window->StateStorage.GetFloat(hover_id, hover_target);
        hover_t = ImLerp(hover_t, hover_target, delta_time * anim_speed);
        window->StateStorage.SetFloat(hover_id, hover_t);

        // 스텝 색상 보간 (비활성 -> 호버 -> 활성)
        ImVec4 base_col = ImLerp(inactive_col_v, hover_col_v, hover_t);
        ImVec4 final_circle_col_v = ImLerp(base_col, active_col_v, anim_t);
        ImU32 current_circle_col = GetColorU32(final_circle_col_v);

        // 활성화/클릭 스케일(크기 변화) 연출: 활성화될 때 살짝 커졌다가 정착
        float current_radius = circle_radius + (anim_t * 1.5f);
        if (held && hovered) current_radius -= 1.0f; // 누르는 동안 살짝 작아짐

        // 원 그리기
        draw_list->AddCircleFilled(center, current_radius, current_circle_col);

        // 체크 아이콘 드로잉 애니메이션 (anim_t 진행도에 맞춰 선이 그려짐)
        if (anim_t)
        {
            float check_thickness = 2.5f;
            ImVec2 p1 = ImVec2(center.x - 5.0f, center.y + 1.0f);
            ImVec2 p2 = ImVec2(center.x - 1.5f, center.y + 4.5f);
            ImVec2 p3 = ImVec2(center.x + 5.5f, center.y - 3.5f);

            // 획 그려지는 애니메이션 (0.1 ~ 0.5 구간: p1->p2, 0.5 ~ 1.0 구간: p2->p3)
            float check_t = ImClamp((anim_t - 0.1f) / 0.9f, 0.0f, 1.0f);

            draw_list->PathLineTo(p1);
            if (check_t <= 0.4f)
            {
                float t1 = check_t / 0.4f;
                draw_list->PathLineTo(ImLerp(p1, p2, t1));
            }
            else
            {
                draw_list->PathLineTo(p2);
                float t2 = (check_t - 0.4f) / 0.6f;
                draw_list->PathLineTo(ImLerp(p2, p3, t2));
            }

            // 알파(투명도) 보간 적용하여 아이콘 표시
            ImU32 alpha_icon_col = (icon_col & ~IM_COL32_A_MASK) | ((ImU32)(255 * check_t) << IM_COL32_A_SHIFT);
            draw_list->PathStroke(alpha_icon_col, 0, check_thickness);
        }

        // 하단 텍스트 렌더링 (활성/비활성 색상 Lerp)
        if (step_labels && step_labels[i])
        {
            const char* text = step_labels[i];
            ImVec2 text_pos = ImVec2(center.x, center.y + circle_radius + 12.0f);

            ImU32 current_text_col = GetColorU32(ImLerp(
                ColorConvertU32ToFloat4(text_inactive_col),
                ColorConvertU32ToFloat4(text_active_col),
                anim_t
            ));

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

