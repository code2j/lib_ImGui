#pragma once
#include "ui_widgets.h"
#include "imgui_internal.h"


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

    // [수정 핵심 1] 화면 밖으로 벗어났다고 바로 return false를 하면 안 됩니다!
    // is_visible 상태만 저장하고, 클릭이나 텍스트 렌더링만 스킵해야 합니다.
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

        // 우측 끝 V자 화살표(Chevron) 렌더링
        ImVec2 chevron_center = ImVec2(bb.Max.x - padding.x - g.FontSize * 0.5f, bb.Min.y + frame_height * 0.5f);
        float chevron_size = 6.5f;
        ImVec2 p1, p2, p3;

        if (is_open) {
            p1 = ImVec2(chevron_center.x - chevron_size, chevron_center.y + chevron_size * 0.4f);
            p2 = ImVec2(chevron_center.x, chevron_center.y - chevron_size * 0.6f);
            p3 = ImVec2(chevron_center.x + chevron_size, chevron_center.y + chevron_size * 0.4f);
        } else {
            p1 = ImVec2(chevron_center.x - chevron_size, chevron_center.y - chevron_size * 0.4f);
            p2 = ImVec2(chevron_center.x, chevron_center.y + chevron_size * 0.6f);
            p3 = ImVec2(chevron_center.x + chevron_size, chevron_center.y - chevron_size * 0.4f);
        }

        ImVec2 points[3] = { p1, p2, p3 };
        window->DrawList->AddPolyline(points, 3, text_col, 0, 2.5f);

        // 텍스트 좌측 정렬 및 클리핑 렌더링
        ImVec2 text_pos(window->WorkRect.Min.x + padding.x, bb.Min.y + padding.y);
        ImVec2 clip_rect_max = ImVec2(bb.Max.x - padding.x * 2.0f - g.FontSize, bb.Max.y);
        ImGui::RenderTextClipped(text_pos, clip_rect_max, label, NULL, NULL);
    }

    // =========================================================================

    // 헤더가 화면 밖에 있어도 애니메이션 프레임은 무조건 업데이트 되어야 함
    if (!calculating_height) {
        float speed = ImGui::GetIO().DeltaTime * 7.0f;
        anim_t = ImClamp(anim_t + (is_open ? speed : -speed), 0.0f, 1.0f);
        window->StateStorage.SetFloat(id + 1, anim_t);
    }

    // 내용 영역 (Child Window) 열기
    // 헤더가 화면에 안 보여도 내용물(Child) 공간은 계속 유지되어야 스크롤이 튕기지 않음
    if (anim_t > 0.0f || calculating_height) {
        float current_height = calculating_height ? 0.0f : (max_height * anim_t);
        float alpha = calculating_height ? 0.0f : anim_t;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().ChildRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGuiChildFlags child_flags = 0;

        if (calculating_height || (is_open && anim_t >= 1.0f)) {
            // [수정 핵심 2] 가로 크기 피드백 루프 방지를 위해 AutoResizeY(세로)만 적용
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

    // 왼쪽 그랩 (▶ 모양)
    window->DrawList->AddTriangleFilled(
        ImVec2(lx - tri_w, track_y - tri_h),
        ImVec2(lx + tri_w, track_y),
        ImVec2(lx - tri_w, track_y + tri_h),
        grab_col);

    // 오른쪽 그랩 (◀ 모양)
    window->DrawList->AddTriangleFilled(
        ImVec2(rx + tri_w, track_y - tri_h),
        ImVec2(rx + tri_w, track_y + tri_h),
        ImVec2(rx - tri_w, track_y),
        grab_col);

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

