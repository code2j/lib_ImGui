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
    // 헤더 UI 영역 계산 (양끝 여백 없애기)
    // =========================================================================
    ImVec2 pos = window->DC.CursorPos;
    float frame_height = ImGui::GetFrameHeight();

    ImRect bb;
    bb.Min.x = window->WorkRect.Min.x; // 현재 작업 영역의 시작점
    bb.Max.x = window->WorkRect.Max.x; // 현재 작업 영역의 끝점
    bb.Min.y = pos.y;
    bb.Max.y = pos.y + frame_height;

    // ImGui의 기본 CollapsingHeader처럼 양끝으로 배경을 살짝 더 확장 (여백 제거)
    const float outer_extend = IM_TRUNC(window->WindowPadding.x * 0.5f);
    bb.Min.x -= outer_extend;
    bb.Max.x += outer_extend;

    // 아이템 크기 등록
    ImGui::ItemSize(ImVec2(window->WorkRect.Max.x - pos.x, frame_height));
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

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
    // [수정됨] 텍스트와 화살표 위치 정렬
    // =========================================================================

    // 1. 우측 끝 V자 화살표(Chevron) 렌더링
    // -> window->WorkRect.Max.x 대신 확장된 배경인 bb.Max.x를 사용하여 위치 일치
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

    // 2. 텍스트 좌측 정렬 및 클리핑 렌더링
    // -> 클리핑 영역(clip_rect_max) 역시 bb.Max.x 기준으로 넓혀서 화살표 직전까지 텍스트가 보이게 함
    ImVec2 text_pos(window->WorkRect.Min.x + padding.x, bb.Min.y + padding.y);
    ImVec2 clip_rect_max = ImVec2(bb.Max.x - padding.x * 2.0f - g.FontSize, bb.Max.y);
    ImGui::RenderTextClipped(text_pos, clip_rect_max, label, NULL, NULL);

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
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGuiChildFlags child_flags = 0;

        if (calculating_height || (is_open && anim_t >= 1.0f)) {
            child_flags |= ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY;
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
