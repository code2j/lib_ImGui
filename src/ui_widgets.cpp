#pragma once
#include "ui_widgets.h"
#include "imgui_internal.h"
#include <cstdint>

#include "ui.hpp"


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

bool ImGui::ButtonX(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderNavCursor(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");

    // PushStyleColor(ImGuiCol_Text, IM_COL32(228, 228, 230, 255));
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
    // PopStyleColor();

    // Automatically close popups
    //if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
    //    CloseCurrentPopup();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
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

    ImU32 bg_track_col = GetColorU32(ImGuiCol_ScrollbarGrab); //IM_COL32(65, 65, 70, 255);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    window->DrawList->AddRectFilled(
        ImVec2(lx, track_min.y),
        ImVec2(rx, track_max.y),
        fill_track_col, track_height * 0.5f);

    ImU32 grab_col = IM_COL32(255, 255, 255, 255);
    ImU32 grab_border_col = GetColorU32(ImGuiCol_Border);
    float grab_border_thickness = 1.0f;

    float tri_w = 7.0f;
    float tri_h = 9.0f;

    auto AddRoundedTriangle = [](ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImVec2 p3, float radius, ImU32 fill_col, ImU32 border_col, float border_thickness)
    {
        // 경로를 생성하는 부분을 람다로 묶음 (채우기와 테두리에 각각 사용하기 위함)
        auto build_path = [&]() {
            ImVec2 pts[3] = { p1, p2, p3 };
            for (int i = 0; i < 3; i++)
            {
                ImVec2 prev = pts[(i + 2) % 3];
                ImVec2 curr = pts[i];
                ImVec2 next = pts[(i + 1) % 3];

                ImVec2 v1 = ImVec2(prev.x - curr.x, prev.y - curr.y);
                ImVec2 v2 = ImVec2(next.x - curr.x, next.y - curr.y);

                float len1 = std::sqrt(v1.x * v1.x + v1.y * v1.y);
                float len2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
                if (len1 > 0.0f) { v1.x /= len1; v1.y /= len1; }
                if (len2 > 0.0f) { v2.x /= len2; v2.y /= len2; }

                float angle1 = std::atan2(v1.y, v1.x);
                float angle2 = std::atan2(v2.y, v2.x);

                float diff = angle2 - angle1;
                while (diff < -IM_PI) diff += IM_PI * 2.0f;
                while (diff > IM_PI) diff -= IM_PI * 2.0f;

                angle2 = angle1 + diff;

                draw_list->PathArcTo(curr, radius, angle1, angle2, 6);
            }
        };

        // 1. 내부 칠하기
        build_path();
        draw_list->PathFillConvex(fill_col);

        // 2. 테두리(보더) 그리기
        if (border_thickness > 0.0f)
        {
            build_path();
            draw_list->PathStroke(border_col, ImDrawFlags_Closed, border_thickness);
        }
    };

    float corner_radius = 2.0f; // 둥근 모서리 반지름 (1.5f ~ 2.5f 추천)

    // 왼쪽 그랩 (▶ 둥근 모양 + 보더)
    AddRoundedTriangle(
        window->DrawList,
        ImVec2(lx - tri_w, track_y - tri_h),
        ImVec2(lx + tri_w, track_y),
        ImVec2(lx - tri_w, track_y + tri_h),
        corner_radius,
        grab_col, grab_border_col, grab_border_thickness
    );

    // 오른쪽 그랩 (◀ 둥근 모양 + 보더)
    AddRoundedTriangle(
        window->DrawList,
        ImVec2(rx + tri_w, track_y - tri_h),
        ImVec2(rx + tri_w, track_y + tri_h),
        ImVec2(rx - tri_w, track_y),
        corner_radius,
        grab_col, grab_border_col, grab_border_thickness
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

        ImU32 tooltip_bg_col     = GetColorU32(ImGuiCol_FrameBg);
        ImU32 tooltip_border_col = GetColorU32(ImGuiCol_Border);

        // 1. 말풍선 둥근 사각형 배경 & 테두리
        window->DrawList->AddRectFilled(tooltip_min, tooltip_max, tooltip_bg_col, 6.0f);
        window->DrawList->AddRect(tooltip_min, tooltip_max, tooltip_border_col, 6.0f);

        // 꼬리 꼭짓점 기본 좌표 (사각형 하단 선 기준)
        ImVec2 p1 = ImVec2(active_x - 6.0f, tooltip_max.y);
        ImVec2 p2 = ImVec2(active_x + 6.0f, tooltip_max.y);
        ImVec2 p3 = ImVec2(active_x, tooltip_max.y + 6.0f); // 꼬리 끝(아래)

        // 2. 몸통과 꼬리가 만나는 부분의 테두리를 확실하게 지우기
        // 높이 2px짜리 배경색 사각형을 테두리 위에 덮어씌워 잔상을 없앱니다.
        window->DrawList->AddRectFilled(
            ImVec2(p1.x + 0.5f, tooltip_max.y - 1.0f),
            ImVec2(p2.x - 0.5f, tooltip_max.y + 1.0f),
            tooltip_bg_col
        );

        // 3. 꼬리 배경 삼각형 그리기
        // 틈새가 안 생기도록 삼각형 윗변을 사각형 안쪽(위)으로 1픽셀 밀어 올려서 그립니다.
        ImVec2 fill_p1 = ImVec2(p1.x, tooltip_max.y - 1.0f);
        ImVec2 fill_p2 = ImVec2(p2.x, tooltip_max.y - 1.0f);
        window->DrawList->AddTriangleFilled(fill_p1, fill_p2, p3, tooltip_bg_col);

        // 4. 꼬리 테두리 (V자 선)
        // 테두리 선이 사각형 바닥선에서 딱 떨어지게 연결됩니다.
        ImVec2 tail_pts[3] = { p1, p3, p2 };
        window->DrawList->AddPolyline(tail_pts, 3, tooltip_border_col, 0, 1.0f);

        // 5. 텍스트 렌더링
        window->DrawList->AddText(tooltip_min + padding, GetColorU32(ImGuiCol_Text), value_buf);
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
        if (ImGuiExt::theme_id == 1) { // Dark 테마
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(31.0f / 255.0f, 31.0f / 255.0f, 32.0f / 255.0f, 1.0f),    // Hovered OFF (어두운 회색)
                ImVec4(113.0f / 255.0f, 125.0f / 255.0f, 255.0f / 255.0f, 1.0f), // Hovered ON (밝은 푸른색)
                anim_t));
        }
        else { // White 테마
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(185.0f / 255.0f, 195.0f / 255.0f, 235.0f / 255.0f, 1.0f), // Hovered OFF (조금 더 진한 흐린 푸른색)
                ImVec4(113.0f / 255.0f, 125.0f / 255.0f, 255.0f / 255.0f, 1.0f), // Hovered ON (밝은 푸른색)
                anim_t));
        }
    }
    else
    {
        if (ImGuiExt::theme_id == 1) { // Dark 테마
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(11.0f / 255.0f, 11.0f / 255.0f, 12.0f / 255.0f, 1.0f),   // Normal OFF (매우 어두운 회색)
                ImVec4(93.0f / 255.0f, 105.0f / 255.0f, 240.0f / 255.0f, 1.0f), // Normal ON (푸른색)
                anim_t));
        }
        else { // White 테마
            col_bg = ImGui::GetColorU32(ImLerp(
                ImVec4(205.0f / 255.0f, 215.0f / 255.0f, 245.0f / 255.0f, 1.0f), // Normal OFF (연하고 흐린 푸른색)
                ImVec4(93.0f / 255.0f, 105.0f / 255.0f, 240.0f / 255.0f, 1.0f),  // Normal ON (푸른색)
                anim_t));
        }
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
    ImVec4 active_col_v     = ImGuiExt::theme_id ? ImColor(87, 242, 135):ImColor(98, 192, 115);    // 부드러운 그린 (활성)
    ImVec4 inactive_col_v   = ImGuiExt::theme_id ? ImColor(36, 36, 49) : ImColor(200, 200, 200);   // 다크 그레이 (비활성) [dark/white]
    ImVec4 hover_col_v      = ImGuiExt::theme_id ? ImColor(44, 44, 47) : ImColor(180, 180, 180);   // 호버 색상


    ImU32 text_active_col   = ImGuiExt::theme_id ? IM_COL32(240, 240, 240, 255) : IM_COL32(40, 40, 45, 255); // [dark/white]
    ImU32 text_inactive_col = ImGuiExt::theme_id ? IM_COL32(130, 130, 130, 255) : IM_COL32(190, 190, 190, 255);
    ImU32 icon_col          = ImGuiExt::theme_id ? IM_COL32(20, 20, 20, 255)    : IM_COL32(240, 240, 240, 240);

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

bool ImGui::Joystic(ImVec2* out)
{
    // 1. 패드 기본 설정
    ImVec2 pad_size(200.0f, 200.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 상호작용용 InvisibleButton
    ImGui::InvisibleButton("joystick_pad_3dof", pad_size);
    bool is_active = ImGui::IsItemActive();
    bool is_activated = ImGui::IsItemActivated();

    ImVec2 center = ImVec2(p.x + pad_size.x * 0.5f, p.y + pad_size.y * 0.5f);

    // 반지름 정의
    float outer_ring_radius = pad_size.x * 0.5f - 5.0f;  // Yaw 링 외각
    float inner_ring_radius = outer_ring_radius - 20.0f; // Yaw 링 내각
    float move_radius = inner_ring_radius - 2.0f;        // XY 이동 제한 영역
    float handle_radius = 12.0f;                         // 중앙 작은 원(핸들)의 클릭 히트박스 반지름

    ImVec2 handle_pos = center;
    float output_x = 0.0f;
    float output_y = 0.0f;

    // 조작 상태 및 초기 마우스 클릭 위치
    static bool is_dragging_handle = false;
    static ImVec2 drag_start_mouse_pos = ImVec2(0.0f, 0.0f);

    // -------------------------------------------------------------
    // 클릭 시작 시: 중앙 작은 원(핸들) 안을 눌렀는지 검사
    // -------------------------------------------------------------
    if (is_activated) {
        ImVec2 m_pos = ImGui::GetIO().MousePos;

        // 클릭한 위치와 중앙 핸들(center) 간의 거리 계산
        float dist_to_handle = sqrtf((m_pos.x - center.x) * (m_pos.x - center.x) +
                                     (m_pos.y - center.y) * (m_pos.y - center.y));

        // 핸들 영역(작은 원) 내부를 클릭했을 때만 드래그 허용
        if (dist_to_handle <= handle_radius + 3.0f) { // 약간의 판정 여유(+3.0f) 부여
            is_dragging_handle = true;
            drag_start_mouse_pos = m_pos; // 첫 클릭 위치 기록 (상대 이동용)
        } else {
            is_dragging_handle = false;
        }
    }

    // 마우스를 떼면 상태 초기화 (중앙 복귀)
    if (!is_active) {
        is_dragging_handle = false;
    }

    // -------------------------------------------------------------
    // 핸들을 성공적으로 클릭한 상태에서 드래그 처리
    // -------------------------------------------------------------
    if (is_active && is_dragging_handle) {
        ImVec2 current_mouse_pos = ImGui::GetIO().MousePos;

        // 상대적 이동량 (Delta)
        ImVec2 offset = ImVec2(current_mouse_pos.x - drag_start_mouse_pos.x,
                               current_mouse_pos.y - drag_start_mouse_pos.y);

        // XY 이동 제한 (move_radius 범위)
        float len_sq = offset.x * offset.x + offset.y * offset.y;
        if (len_sq > move_radius * move_radius && len_sq > 0.0f) {
            float len = sqrt(len_sq);
            offset.x = (offset.x / len) * move_radius;
            offset.y = (offset.y / len) * move_radius;
        }

        handle_pos = ImVec2(center.x + offset.x, center.y + offset.y);

        // 출력 좌표 계산 (-1.0f ~ 1.0f)
        output_x = offset.x / move_radius;
        output_y = -offset.y / move_radius; // Y축 상하 반전
    }

    // -------------------------------------------------------------
    // 값 변경 검사 및 포인터 인자 출력 연결
    // -------------------------------------------------------------
    bool value_changed = false;

    if (out) {
        // 이전 값과 비교하여 다를 경우에만 true 설정 및 값 업데이트
        if (out->x != output_x || out->y != output_y) {
            out->x = output_x;
            out->y = output_y;
            value_changed = true;
        }
    } else {
        // out 포인터가 없을 경우 내부 값이 0이 아니면 조작 중인 것으로 간주
        if (output_x != 0.0f || output_y != 0.0f) {
            value_changed = true;
        }
    }

    // -------------------------------------------------------------
    // 그래픽 렌더링
    // -------------------------------------------------------------
    // 1. 배경 패드 (4각형)
    draw_list->AddRectFilled(p, ImVec2(p.x + pad_size.x, p.y + pad_size.y), IM_COL32(35, 35, 38, 255), 10.0f);
    draw_list->AddRect(p, ImVec2(p.x + pad_size.x, p.y + pad_size.y), IM_COL32(70, 70, 80, 255), 10.0f, 0, 1.5f);

    // 2. XY 이동 가이드 영역 (큰 원)
    draw_list->AddCircleFilled(center, move_radius, IM_COL32(45, 45, 50, 255), 64);
    draw_list->AddCircle(center, move_radius, IM_COL32(80, 80, 90, 255), 64, 1.0f);

    // 3. 십자 가이드선
    draw_list->AddLine(ImVec2(center.x - 8.0f, center.y), ImVec2(center.x + 8.0f, center.y), IM_COL32(100, 100, 100, 255));
    draw_list->AddLine(ImVec2(center.x, center.y - 8.0f), ImVec2(center.x, center.y + 8.0f), IM_COL32(100, 100, 100, 255));

    // 4. 중앙 작은 원 (조이스틱 핸들)
    ImU32 handle_color = is_dragging_handle ? IM_COL32(66, 150, 250, 255) : IM_COL32(200, 200, 200, 255);
    draw_list->AddCircleFilled(handle_pos, handle_radius, handle_color);
    draw_list->AddCircle(handle_pos, handle_radius, IM_COL32(255, 255, 255, 255), 0, 1.5f);

    // -------------------------------------------------------------
    // 패드 내부 하단 중앙에 가로 텍스트 정렬
    // -------------------------------------------------------------
    char text_buf[64];
    snprintf(text_buf, sizeof(text_buf), "X: %.2f  |  Y: %.2f", output_x, output_y);

    ImVec2 text_size = ImGui::CalcTextSize(text_buf);

    // 패드 하단 중앙 위치 계산 (패드 바닥에서 10px 위)
    float text_x = center.x - (text_size.x * 0.5f);
    float text_y = p.y + pad_size.y - text_size.y - 5.0f;
    ImVec2 text_pos = ImVec2(text_x, text_y);

    // 반투명 패널 (가독성 보장)
    draw_list->AddRectFilled(
        ImVec2(text_pos.x - 6.0f, text_pos.y - 2.0f),
        ImVec2(text_pos.x + text_size.x + 6.0f, text_pos.y + text_size.y + 2.0f),
        IM_COL32(20, 20, 22, 100),
        4.0f
    );

    // 텍스트 출력
    draw_list->AddText(text_pos, IM_COL32(220, 220, 220, 255), text_buf);

    // 최종적으로 값 변경 여부를 리턴
    return value_changed;
}


bool ImGui::TransformControl(Eigen::Matrix4d* matrix)
{
    if (!matrix) {
        return false;
    }

    bool is_changed = false;

    ImGui::PushID(matrix);

    // 위젯 내부에서 상태를 유지할 변수
    static float translation[3]  = { 0.0f, 0.0f, 0.0f };
    static float rotation_deg[3] = { 0.0f, 0.0f, 0.0f };


    // -------------------------------------------------------------
    // 초기화 버튼
    // -------------------------------------------------------------
    if (ImGui::Button("Reset All", ImVec2(-1, 0))) {
        for(int i = 0; i < 3; ++i) {
            translation[i] = 0.0f;
            rotation_deg[i] = 0.0f;
        }
        is_changed = true;
    }

    ImGui::Spacing();


    // -------------------------------------------------------------
    // 이동 (Translation) 컨트롤
    // -------------------------------------------------------------
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Translation (X, Y, Z)");
    ImGui::PushItemWidth(-1);
    if (ImGui::DragFloat3("##Trans", translation, 0.01f, 0.0f, 0.0f, "%.3f")) {
        is_changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();


    // -------------------------------------------------------------
    // 회전 (Rotation) 컨트롤
    // -------------------------------------------------------------
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Rotation [Deg] (Roll, Pitch, Yaw)");
    ImGui::PushItemWidth(-1);
    if (ImGui::DragFloat3("##Rot", rotation_deg, 0.5f, 0.0f, 0.0f, "%.1f")) {
        is_changed = true;
    }
    ImGui::PopItemWidth();


    // -------------------------------------------------------------
    // 변경 시 Matrix 업데이트
    // -------------------------------------------------------------
    if (is_changed) {
        double roll_rad  = rotation_deg[0] * (M_PI / 180.0);
        double pitch_rad = rotation_deg[1] * (M_PI / 180.0);
        double yaw_rad   = rotation_deg[2] * (M_PI / 180.0);

        Eigen::AngleAxisd rollAngle(roll_rad, Eigen::Vector3d::UnitX());
        Eigen::AngleAxisd pitchAngle(pitch_rad, Eigen::Vector3d::UnitY());
        Eigen::AngleAxisd yawAngle(yaw_rad, Eigen::Vector3d::UnitZ());

        Eigen::Matrix3d rotation_matrix = (yawAngle * pitchAngle * rollAngle).matrix();

        // 포인터 접근(->)을 통한 행렬 조작
        matrix->setIdentity();
        matrix->block<3, 3>(0, 0) = rotation_matrix;
        matrix->block<3, 1>(0, 3) = Eigen::Vector3d(translation[0], translation[1], translation[2]);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();


    // -------------------------------------------------------------
    // 결과 행렬 출력
    // -------------------------------------------------------------
    ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Current Matrix (4x4)");
    if (ImGui::BeginTable("MatrixTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
        for (int i = 0; i < 4; ++i) {
            ImGui::TableNextRow();
            for (int j = 0; j < 4; ++j) {
                ImGui::TableSetColumnIndex(j);
                // 괄호와 역참조 연산자(*matrix)를 사용하여 요소 접근
                if (j == 3 && i != 3) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%8.3f", (*matrix)(i, j));
                } else {
                    ImGui::Text("%8.3f", (*matrix)(i, j));
                }
            }
        }
        ImGui::EndTable();
    }

    ImGui::PopID();

    return is_changed;
}

bool ImGui::ThemeSelector(int* current_theme)
{
    bool value_changed = false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // 두 가지 테마 정의 (Light / Dark)
    ImGui::ThemePreviewData themes[2] = {
        {
            "Light Mode",
            ImColor(251, 251, 251),  // Bg
            ImColor(235, 235, 237),   // Panel
            ImColor(93, 105, 240),    // Primary
            ImColor(246, 246, 246),   // Secondary
            ImColor(40, 40, 45)       // Text
        },
        {
            "Dark Mode",
            ImColor(7, 7, 9),       // Bg
            ImColor(12, 12, 14),   // Panel
            ImColor(93, 105, 240), // Primary
            ImColor(11, 11, 12),   // Secondary
            ImColor(228, 228, 230) // Text
        }
    };

    // 카드 크기 설정
    const ImVec2 card_size(140.0f, 130.0f);
    const float preview_h = 90.0f; // 상단 미리보기 영역 높이
    const float rounding = 8.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16.0f, 10.0f));

    for (int i = 0; i < 2; i++)
    {
        if (i > 0) ImGui::SameLine();

        ImGui::PushID(i);

        const ImGui::ThemePreviewData& theme = themes[i];
        bool is_selected = (*current_theme == i);

        // 카드의 Bounding Box 계산
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + card_size.x, pos.y + card_size.y));

        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, window->GetID("##ThemeCard")))
        {
            ImGui::PopID();
            continue;
        }

        // 클릭 이벤트 처리
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, window->GetID("##ThemeCard"), &hovered, &held);
        if (pressed)
        {
            *current_theme = i;
            value_changed = true;
        }

        // ==========================================
        // 1. 카드 배경 및 테두리 (Hover & Selection)
        // ==========================================
        ImU32 card_bg = ImGui::GetColorU32(ImGuiCol_WindowBg); // 현재 ImGui 테마의 윈도우 배경색 사용
        ImU32 border_col = is_selected ? theme.PrimaryColor : (hovered ? IM_COL32(0, 0, 0, 0) : IM_COL32(0, 0, 0, 0));

        window->DrawList->AddRectFilled(bb.Min, bb.Max, card_bg, rounding);
        window->DrawList->AddRect(bb.Min, bb.Max, border_col, rounding, 0, is_selected ? 2.0f : 1.0f);

        // ==========================================
        // 2. 미니 UI 미리보기 (상단)
        // ==========================================
        ImVec2 p_min = ImVec2(bb.Min.x + 2.0f, bb.Min.y + 2.0f);
        ImVec2 p_max = ImVec2(bb.Max.x - 2.0f, bb.Min.y + preview_h);

        // 미리보기 전체 배경
        window->DrawList->AddRectFilled(p_min, p_max, theme.BgColor, rounding - 2.0f, ImDrawFlags_RoundCornersTop);

        // 가짜 UI: 상단 헤더 (Header)
        ImVec2 header_p_max = ImVec2(p_max.x, p_min.y + 16.0f);
        window->DrawList->AddRectFilled(p_min, header_p_max, theme.PanelColor, rounding - 2.0f, ImDrawFlags_RoundCornersTop);
        // 헤더 안의 가짜 텍스트 (타이틀)
        window->DrawList->AddRectFilled(ImVec2(p_min.x + 8.0f, p_min.y + 6.0f), ImVec2(p_min.x + 40.0f, p_min.y + 10.0f), theme.TextColor);

        // 가짜 UI: 좌측 사이드바 (Sidebar)
        ImVec2 sidebar_p_max = ImVec2(p_min.x + 30.0f, p_max.y);
        window->DrawList->AddRectFilled(ImVec2(p_min.x, header_p_max.y + 4.0f), sidebar_p_max, theme.PanelColor, 0.0f);
        // 사이드바 안의 가짜 메뉴 아이템들
        for (int j = 0; j < 3; j++) {
            float sy = header_p_max.y + 12.0f + (j * 12.0f);
            window->DrawList->AddRectFilled(ImVec2(p_min.x + 6.0f, sy), ImVec2(p_min.x + 24.0f, sy + 4.0f), theme.TextColor);
        }

        // 가짜 UI: 메인 컨텐츠 영역 패널
        ImVec2 content_min = ImVec2(p_min.x + 38.0f, header_p_max.y + 12.0f);
        ImVec2 content_max = ImVec2(p_max.x - 8.0f, p_max.y - 8.0f);
        window->DrawList->AddRectFilled(content_min, content_max, theme.PanelColor, 4.0f);

        // 가짜 UI: Primary Button
        ImVec2 btn1_min = ImVec2(content_min.x + 8.0f, content_min.y + 8.0f);
        ImVec2 btn1_max = ImVec2(content_min.x + 40.0f, content_min.y + 20.0f);
        window->DrawList->AddRectFilled(btn1_min, btn1_max, theme.PrimaryColor, 3.0f);

        // 가짜 UI: Secondary Button / Badge
        ImVec2 btn2_min = ImVec2(btn1_max.x + 6.0f, content_min.y + 8.0f);
        ImVec2 btn2_max = ImVec2(btn2_min.x + 18.0f, content_min.y + 20.0f);
        window->DrawList->AddRectFilled(btn2_min, btn2_max, theme.SecondaryColor, 3.0f);

        // 가짜 UI: 본문 텍스트 줄
        for (int j = 0; j < 2; j++) {
            float ty = btn1_max.y + 10.0f + (j * 8.0f);
            float width = (j == 0) ? 35.0f : 20.0f; // 두 번째 줄은 짧게
            window->DrawList->AddRectFilled(ImVec2(content_min.x + 8.0f, ty), ImVec2(content_min.x + 8.0f + width, ty + 3.0f), theme.TextColor);
        }

        // 미리보기와 아래 텍스트 사이의 구분선
        window->DrawList->AddLine(ImVec2(bb.Min.x, p_max.y), ImVec2(bb.Max.x, p_max.y), IM_COL32(100, 100, 100, 50));

        // ==========================================
        // 3. 하단 테마 이름 텍스트
        // ==========================================
        ImVec2 text_size = ImGui::CalcTextSize(theme.Name);
        ImVec2 text_pos = ImVec2(
            bb.Min.x + (card_size.x - text_size.x) * 0.5f,
            bb.Min.y + preview_h + ((card_size.y - preview_h) - text_size.y) * 0.5f
        );

        // 선택된 상태면 Primary 색상으로, 아니면 기본 텍스트 색상으로 출력
        ImU32 name_col = is_selected ? theme.PrimaryColor : ImGui::GetColorU32(ImGuiCol_Text);
        window->DrawList->AddText(text_pos, name_col, theme.Name);

        ImGui::PopID();
    }

    ImGui::PopStyleVar();

    return value_changed;
}


