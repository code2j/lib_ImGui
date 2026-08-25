#pragma once
#include "ui_widgets.h"
#include "imgui_internal.h"
#include <cstdint>

#include "ui.hpp"

static const float DRAGDROP_HOLD_TO_OPEN_TIMER = 0.70f;    // Time for drag-hold to activate items accepting the ImGuiButtonFlags_PressedOnDragDropHold button behavior.
static const float DRAG_MOUSE_THRESHOLD_FACTOR = 0.50f;
static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}
// Getter for the old Combo() API: "item1\0item2\0item3\0"
static const char* Items_SingleStringGetter(void* data, int idx)
{
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += ImStrlen(p) + 1;
        items_count++;
    }
    return *p ? p : NULL;
}


static ImVector<ImGuiID> s_CollapsingHeaderIdStack;

bool ImGui::BeginCollapsingHeader(const char* label, bool default_open)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiID id = window->GetID(label);

    // 상태 및 애니메이션 변수 가져오기
    int stored_is_open = window->StateStorage.GetInt(id, -1); // -1은 초기화되지 않음을 의미
    if (stored_is_open == -1) {
        stored_is_open = default_open ? 1 : 0;
        window->StateStorage.SetInt(id, stored_is_open); // 처음 한 번 Storage에 기록!
    }

    bool is_open = (stored_is_open != 0);
    float anim_t = window->StateStorage.GetFloat(id + 1, is_open ? 1.0f : 0.0f);
    float max_height = window->StateStorage.GetFloat(id + 2, 0.0f);

    bool calculating_height = (is_open && max_height == 0.0f);

    // 헤더 UI 영역 계산
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

    ImGui::ItemSize(ImVec2(window->WorkRect.Max.x - pos.x, frame_height));
    bool is_visible = ImGui::ItemAdd(bb, id);

    if (is_visible) {
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed) {
            is_open = !is_open;
            window->StateStorage.SetInt(id, is_open ? 1 : 0);
        }

        ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        ImGui::RenderFrame(bb.Min, bb.Max, bg_col, false, ImGui::GetStyle().FrameRounding);

        ImVec2 padding = ImGui::GetStyle().FramePadding;
        ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);

        ImVec2 chevron_center = ImVec2(bb.Max.x - padding.x - g.FontSize * 0.5f, bb.Min.y + frame_height * 0.5f);
        float chevron_size = 5.0f;
        ImVec2 p1, p2, p3;

        if (is_open) {
            p1 = ImVec2(chevron_center.x - chevron_size, chevron_center.y - chevron_size * 0.4f);
            p2 = ImVec2(chevron_center.x, chevron_center.y + chevron_size * 0.6f);
            p3 = ImVec2(chevron_center.x + chevron_size, chevron_center.y - chevron_size * 0.4f);
        } else {
            p1 = ImVec2(chevron_center.x - chevron_size * 0.4f, chevron_center.y - chevron_size);
            p2 = ImVec2(chevron_center.x + chevron_size * 0.6f, chevron_center.y);
            p3 = ImVec2(chevron_center.x - chevron_size * 0.4f, chevron_center.y + chevron_size);
        }

        ImVec2 points[3] = { p1, p2, p3 };
        window->DrawList->AddPolyline(points, 3, text_col, 0, 2.0f);

        ImVec2 text_pos(window->WorkRect.Min.x + padding.x, bb.Min.y + padding.y);
        ImVec2 clip_rect_max = ImVec2(bb.Max.x - padding.x * 2.0f - g.FontSize, bb.Max.y);
        ImGui::RenderTextClipped(text_pos, clip_rect_max, label, NULL, NULL);
    }

    if (!calculating_height) {
        float speed = ImGui::GetIO().DeltaTime * 7.0f;
        anim_t = ImClamp(anim_t + (is_open ? speed : -speed), 0.0f, 1.0f);
        window->StateStorage.SetFloat(id + 1, anim_t);
    }

    // 내용 영역 (Child Window) 열기
    if (anim_t > 0.0f || calculating_height) {

        float current_height;

        if (calculating_height) {
            // [핵심 수정] 측정 프레임:
            // 0.0f를 넣으면 남은 화면을 꽉 채워버리므로, 눈에 보이지 않는 0.01f로 고정합니다.
            current_height = 0.01f;
        }
        else if (is_open && anim_t >= 1.0f) {
            // 완전히 다 열렸을 때:
            // AutoResizeY가 작동해야 하므로 이때만 0.0f를 허용합니다.
            current_height = 0.0f;
        }
        else {
            // 애니메이션 진행 중:
            current_height = max_height * anim_t;
            // 닫히기 직전이나 연산 오차로 0.0f가 되는 것을 방지
            if (current_height <= 0.01f) {
                current_height = 0.01f;
            }
        }

        // 측정 프레임에는 렌더링 찌꺼기가 보이지 않게 투명하게 처리
        float alpha = calculating_height ? 0.0f : anim_t;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().ChildRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        ImGuiChildFlags child_flags = 0;

        // 측정이 끝났고, 완전히 열렸을 때만 AutoResizeY 적용
        if (!calculating_height && is_open && anim_t >= 1.0f) {
            child_flags |= ImGuiChildFlags_AutoResizeY;
        }

        // 크기가 0.0f(화면 채우기)로 잘못 들어갈 일이 원천 차단됨
        ImGui::BeginChild(id + 3, ImVec2(0, current_height), child_flags, window_flags);

        // 스택에 현재 ID 저장 (EndCollapsingHeader에서 사용)
        s_CollapsingHeaderIdStack.push_back(id);

        return true;
    }

    return false;
}

void ImGui::EndCollapsingHeader()
{
    // Begin 없이 End가 호출되는 것을 방지하는 안전장치
    IM_ASSERT(s_CollapsingHeaderIdStack.Size > 0 && "Mismatched BeginCollapsingHeader / EndCollapsingHeader!");

    ImGuiWindow* child_window = ImGui::GetCurrentWindow();
    float height = child_window->DC.CursorMaxPos.y - child_window->Pos.y + ImGui::GetStyle().WindowPadding.y;

    ImGui::EndChild();
    ImGui::PopStyleVar(3);

    // =========================================================================
    // [수정] 부모 창으로 돌아온 후, 스택에서 저장해둔 ID를 꺼내서 사용
    // =========================================================================
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = s_CollapsingHeaderIdStack.back();
    s_CollapsingHeaderIdStack.pop_back();

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

bool ImGui::Check(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float square_sz = GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;

    // 전체 영역 계산 (현재 가용 너비를 꽉 채움)
    const float min_width = square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f);
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    // 전체 레이아웃 차지 영역
    const ImRect total_bb(pos, pos + ImVec2(total_width, label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    const bool is_visible = ItemAdd(total_bb, id);
    const bool is_multi_select = (g.LastItemData.ItemFlags & ImGuiItemFlags_IsMultiSelect) != 0;
    if (!is_visible)
        if (!is_multi_select || !g.BoxSelectState.UnclipMode || !g.BoxSelectState.UnclipRect.Overlaps(total_bb))
        {
            IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
            return false;
        }

    // 체크박스 사각형(위젯)을 오른쪽 끝 위치로 계산
    const float check_x = pos.x + total_width - square_sz;
    const ImRect check_bb(ImVec2(check_x, pos.y), ImVec2(check_x + square_sz - 0.1f, pos.y + square_sz - 0.1f));

    // Range-Selection/Multi-selection support (header)
    bool checked = *v;
    if (is_multi_select)
        MultiSelectItemHeader(id, &checked, NULL);

    // [수정] Toggle/Radio와 동일하게 우측 버튼 영역(check_bb)에서만 상호작용 감지
    bool hovered, held;
    bool pressed = ButtonBehavior(check_bb, id, &hovered, &held);

    // Range-Selection/Multi-selection support (footer)
    if (is_multi_select)
        MultiSelectItemFooter(id, &checked, &pressed);
    else if (pressed)
        checked = !checked;

    if (*v != checked)
    {
        *v = checked;
        pressed = true; // return value
        MarkItemEdited(id);
    }

    const bool mixed_value = (g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0;
    if (is_visible)
    {
        RenderNavCursor(total_bb, id);

        // 배경색 결정 (ON/Mixed: Button 계열, OFF: FrameBg 계열)
        ImU32 frame_col;
        if (*v || mixed_value)
        {
            frame_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                                    : hovered        ? ImGuiCol_ButtonHovered
                                                     : ImGuiCol_Button);
        }
        else
        {
            frame_col = GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                                    : hovered        ? ImGuiCol_FrameBgHovered
                                                     : ImGuiCol_FrameBg);
        }

        RenderFrame(check_bb.Min, check_bb.Max, frame_col, true, style.FrameRounding);

        // 체크 마크 색상 (흰색 통일)
        ImU32 check_col = IM_COL32(255, 255, 255, 255);

        if (mixed_value)
        {
            ImVec2 pad(ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)), ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
        }
        else if (*v)
        {
            const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 6.0f));
            RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
        }
    }

    // 라벨(텍스트)은 가장 왼쪽 시작점에 배치
    const ImVec2 label_pos = ImVec2(pos.x, check_bb.Min.y + style.FramePadding.y);

    if (g.LogEnabled)
        LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
    if (is_visible && label_size.x > 0.0f)
        RenderText(label_pos, label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}

bool ImGui::Radio(const char* label, int* v, int v_button)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float square_sz = GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;

    const float min_width = square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f);
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    const float check_x = pos.x + total_width - square_sz;
    const ImRect check_bb(ImVec2(check_x, pos.y), ImVec2(check_x + square_sz, pos.y + square_sz));

    const ImRect total_bb(pos, pos + ImVec2(total_width, label_size.y + style.FramePadding.y * 2.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    ImVec2 center = check_bb.GetCenter();
    center.x = IM_ROUND(center.x);
    center.y = IM_ROUND(center.y);
    const float radius = (square_sz - 1.0f) * 0.5f;

    bool hovered, held;
    bool pressed = ButtonBehavior(check_bb, id, &hovered, &held);

    if (pressed)
    {
        *v = v_button;
        MarkItemEdited(id);
    }

    const bool active = (*v == v_button);

    RenderNavCursor(total_bb, id);
    const int num_segment = 300;

    // --- 디자인 렌더링 부분 ---
    // 1. 바깥쪽 원 (배경)
    // OFF 상태: FrameBg 계열
    ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                                : hovered        ? ImGuiCol_FrameBgHovered
                                                 : ImGuiCol_FrameBg);

    // ON (선택) 상태: Button 계열 사용 (CheckMark 대신 Button 적용)
    if (active)
    {
        bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive
                              : hovered        ? ImGuiCol_ButtonHovered
                                               : ImGuiCol_Button);
    }

    window->DrawList->AddCircleFilled(center, radius, bg_col, num_segment);

    // 2. 안쪽의 작고 채워진 흰색 원
    if (active)
    {
        const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 3.5f));
        window->DrawList->AddCircleFilled(center, radius - pad, IM_COL32(255, 255, 255, 255), num_segment);
    }

    // 테두리 선
    if (style.FrameBorderSize > 0.0f)
    {
        window->DrawList->AddCircle(center + ImVec2(1, 1), radius, GetColorU32(ImGuiCol_BorderShadow), num_segment, style.FrameBorderSize);
        window->DrawList->AddCircle(center, radius, GetColorU32(ImGuiCol_Border), num_segment, style.FrameBorderSize);
    }

    // 라벨(텍스트) 배치
    ImVec2 label_pos = ImVec2(pos.x, check_bb.Min.y + style.FramePadding.y);

    if (g.LogEnabled)
        LogRenderedText(&label_pos, active ? "(x)" : "( )");
    if (label_size.x > 0.0f)
        RenderText(label_pos, label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool ImGui::_drag_(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();
    const ImU32 color_marker = (g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasColorMarker) ? g.NextItemData.ColorMarker : 0;

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 pos = window->DC.CursorPos;

    // [수정] 전체 가용 영역 및 위젯이 배치될 오른쪽 좌표 계산
    const float min_width = (label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f) + w;
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    // 드래그 위젯(입력부)을 오른쪽 끝으로 배치
    const float frame_x = pos.x + total_width - w;
    const ImRect frame_bb(ImVec2(frame_x, pos.y), ImVec2(frame_x + w, pos.y + label_size.y + style.FramePadding.y * 2.0f));

    // 전체 차지 영역 (라벨부터 위젯 끝까지)
    const ImRect total_bb(pos, ImVec2(pos.x + total_width, frame_bb.Max.y));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Drag turns it into an InputText
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
        const bool make_active = (clicked || double_clicked || g.NavActivateId == id);
        if (make_active && (clicked || double_clicked))
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // (Optional) simple click (without moving) turns Drag into an InputText
        if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
            if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
            {
                g.NavActivateId = id;
                g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
                temp_input_is_active = true;
            }

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp Ctrl+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
        bool clamp_enabled = false;
        if ((flags & ImGuiSliderFlags_ClampOnInput) && (p_min != NULL || p_max != NULL))
        {
            const int clamp_range_dir = (p_min != NULL && p_max != NULL) ? DataTypeCompare(data_type, p_min, p_max) : 0; // -1 when *p_min < *p_max, == 0 when *p_min == *p_max
            if (p_min == NULL || p_max == NULL || clamp_range_dir < 0)
                clamp_enabled = true;
            else if (clamp_range_dir == 0)
                clamp_enabled = DataTypeIsZero(data_type, p_min) ? ((flags & ImGuiSliderFlags_ClampZeroRange) != 0) : true;
        }
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, false, style.FrameRounding);
    if (color_marker != 0 && style.ColorMarkerSize > 0.0f)
        RenderColorComponentMarker(frame_bb, GetColorU32(color_marker), style.FrameRounding);

    // --- 디자인 수정 부분: 활성화 시 커스텀 테두리 색상 적용 ---
    if (g.ActiveId == id)
    {
        PushStyleColor(ImGuiCol_Border, ImVec4(0.364705882f, 0.411764706f, 0.941176471f, 1.00f));

        float active_border_size = style.FrameBorderSize > 0.0f ? style.FrameBorderSize : 1.0f;
        PushStyleVar(ImGuiStyleVar_FrameBorderSize, active_border_size);

        RenderFrameBorder(frame_bb.Min, frame_bb.Max, g.Style.FrameRounding);

        PopStyleVar();
        PopStyleColor();
    }
    else
    {
        RenderFrameBorder(frame_bb.Min, frame_bb.Max, g.Style.FrameRounding);
    }
    // -------------------------------------------------------------

    // Drag behavior
    const bool value_changed = DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, flags);
    if (value_changed)
        MarkItemEdited(id);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_COUNTOF(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    // [수정] 라벨(텍스트)을 가장 왼쪽 위치(pos.x)에 렌더링
    if (label_size.x > 0.0f)
        RenderText(ImVec2(pos.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ImGui::Drag(const char *label, float *v, float v_speed, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _drag_(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
}

bool ImGui::Drag(const char *label, double *v, float v_speed, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _drag_(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, flags);
}

bool ImGui::Drag(const char *label, int *v, float v_speed, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _drag_(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format, flags);
}

bool ImGui::_dropdown_(const char *label, int *current_item, const char *(*getter)(void *user_data, int idx), void *user_data, int items_count, int popup_max_height_in_items)
{
    bool is_dark = true;
    ImVec4 bg_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    if (bg_color.x > (200.f/255.0f) && bg_color.y > (200.f/255.0f) && bg_color.z > (200.f/255.0f) && bg_color.w > (200.f/255.0f))
    {
        is_dark = false;
    }

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;

    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    // --- 좌측 라벨, 우측 위젯 레이아웃 계산 ---
    const ImVec2 pos = window->DC.CursorPos;
    const float w = CalcItemWidth();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    // 전체 가용 너비 및 우측 콤보박스 x좌표 계산
    const float min_width = (label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f) + w;
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);
    const float combo_x = pos.x + total_width - w;

    // 1. 라벨을 제일 왼쪽에 렌더링
    if (label_size.x > 0.0f)
    {
        // Y축 중앙 정렬을 위해 FramePadding.y를 더해줍니다.
        RenderText(ImVec2(pos.x, pos.y + style.FramePadding.y), label);
    }

    // 2. BeginCombo 위젯을 우측 끝으로 밀어내기 위해 커서 X 이동
    window->DC.CursorPos.x = combo_x;
    SetNextItemWidth(w); // 콤보박스 너비를 명시적으로 강제

    // 3. ID 안정성을 위해 PushID로 감싼 후 숨김 라벨("##combo")을 전달
    ImGui::PushID(label);

    // 콤보 박스 및 팝업 상태 체크를 위한 ID 계산
    ImGuiID combo_id = window->GetID("##combo");
    ImGuiID popup_id = ImHashStr("##ComboPopup", 0, combo_id);

    // 1. Popup 열림 상태 확인
    bool is_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);

    // 2. 애니메이션 진행률(anim_t) 업데이트 로직
    float anim_t = window->StateStorage.GetFloat(combo_id + 1, 0.0f);
    if (is_open) {
        float speed = ImGui::GetIO().DeltaTime * 12.0f; // 속도 조절 (높을수록 빠르게 열림)
        anim_t = ImClamp(anim_t + speed, 0.0f, 1.0f);
    } else {
        anim_t = 0.0f; // 팝업이 닫히면 바로 리셋
    }
    window->StateStorage.SetFloat(combo_id + 1, anim_t);

    // 3. 애니메이션 높이(Size Constraint) 계산 및 적용
    if (!(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint))
    {
        float max_height = CalcMaxPopupHeightFromItemCount(popup_max_height_in_items != -1 ? popup_max_height_in_items : items_count);
        float current_height = ImMax(max_height * anim_t, 1.0f); // 0.0f 할당 시 에러 방지
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, current_height));
    }

    // 애니메이션이 진행 중일 때는 스크롤바를 숨겨서 깔끔하게 연출
    bool is_animating = (anim_t > 0.0f && anim_t < 1.0f);
    if (is_animating) {
        PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    }

    // 4. Main Frame 스타일 적용
    if (is_open)
    {
        PushStyleColor(ImGuiCol_Border, ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
        PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    }
    PushStyleColor(ImGuiCol_FrameBg, GetColorU32(ImGuiCol_FrameBg));

    // 5. Popup 창 스타일 및 배경 Alpha 페이드 인 적용
    PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImVec4 popup_bg = ImGui::ColorConvertU32ToFloat4(GetColorU32(ImGuiCol_ChildBg));
    popup_bg.w *= anim_t; // 투명도 애니메이션 반영
    PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertFloat4ToU32(popup_bg));

    // 실제 콤보 박스 렌더링 ("##combo"를 사용해 라벨 숨김)
    bool combo_started = BeginCombo("##combo", preview_value, ImGuiComboFlags_None);

    // 스타일 복구
    PopStyleColor(1); // FrameBg 복구
    if (is_open)
    {
        PopStyleVar(); // FrameBorderSize 복구
        PopStyleColor(); // Border 복구
    }
    if (is_animating) {
        PopStyleVar(); // ScrollbarSize 복구
    }

    if (!combo_started)
    {
        PopStyleColor(); // PopupBg 복구
        PopStyleVar(); // PopupRounding 복구
        ImGui::PopID(); // PushID 해제
        return false;
    }

    // --- 여기서부터 Popup 내부 ---
    PushStyleVar(ImGuiStyleVar_Alpha, anim_t);

    bool value_changed = false;
    ImGuiListClipper clipper;
    clipper.Begin(items_count);
    clipper.IncludeItemByIndex(*current_item);

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const char* item_text = getter(user_data, i);
            if (item_text == NULL)
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);

            // 다크/화이트 테마에 따른 호버 및 액티브 색상 설정
            ImVec4 hover_color  = is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 0.08f) : ImVec4(0.0f, 0.0f, 0.0f, 0.06f);
            ImVec4 active_color = is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 0.12f) : ImVec4(0.0f, 0.0f, 0.0f, 0.10f);

            if (item_selected)
            {
                PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 기본 선택 배경 투명화
                PushStyleColor(ImGuiCol_HeaderHovered, hover_color);
                PushStyleColor(ImGuiCol_HeaderActive, active_color);
            }
            else
            {
                PushStyleColor(ImGuiCol_HeaderHovered, hover_color);
                PushStyleColor(ImGuiCol_HeaderActive, active_color);
            }

            // 변수 이름 중복(위의 pos)을 피하기 위해 item_pos로 변경
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            float avail_width = ImGui::GetContentRegionAvail().x;
            float item_height = ImGui::GetTextLineHeight() + 16.0f;

            if (Selectable("##item", item_selected, ImGuiSelectableFlags_None, ImVec2(0, item_height)))
            {
                value_changed = true;
                *current_item = i;
            }

            // Alpha 채널 계산 (0 ~ 255)
            int alpha_255 = (int)(255.0f * anim_t);

            // 텍스트 렌더링 (Alpha 적용)
            ImVec2 text_pos = ImVec2(item_pos.x + 10.0f, item_pos.y + (item_height - ImGui::GetTextLineHeight()) * 0.5f);
            ImU32 text_col = is_dark ? IM_COL32(230, 230, 230, alpha_255) : IM_COL32(40, 40, 25, alpha_255);
            ImGui::GetWindowDrawList()->AddText(text_pos, text_col, item_text);

            // 체크마크 렌더링 (Alpha 적용)
            if (item_selected)
            {
                ImVec2 check_pos = ImVec2(item_pos.x + avail_width - 24.0f, item_pos.y + (item_height - 14.0f) * 0.5f);
                ImU32 check_col = is_dark ? IM_COL32(255, 255, 255, alpha_255) : IM_COL32(40, 40, 25, alpha_255);
                ImGui::RenderCheckMark(ImGui::GetWindowDrawList(), check_pos, check_col, 14.0f);
                PopStyleColor(3);
            }
            else
            {
                PopStyleColor(2);
            }

            if (item_selected)
                SetItemDefaultFocus();

            PopID();
        }
    }

    PopStyleVar(); // Alpha 복구
    EndCombo();

    PopStyleColor(); // PopupBg 복구
    PopStyleVar(); // PopupRounding 복구

    ImGui::PopID(); // PushID 해제

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool ImGui::DropDown(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p)
    {
        p += ImStrlen(p) + 1;
        items_count++;
    }
    bool value_changed = _dropdown_(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    return value_changed;
}

bool ImGui::SliderRange(const char* label, float* v_min, float* v_max, float v_bound_min, float v_bound_max, const char* format)
{
    return _slider4_(label, ImGuiDataType_Float, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::SliderRange(const char *label, int *v_min, int *v_max, float v_bound_min, float v_bound_max, const char *format)
{
    return _slider4_(label, ImGuiDataType_S32, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::SliderRange(const char *label, double *v_min, double *v_max, float v_bound_min, float v_bound_max, const char *format)
{
    return _slider4_(label, ImGuiDataType_Double, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::_slider4_(const char *label, ImGuiDataType data_type, void *p_min, void *p_max, const void *p_bound_min, const void *p_bound_max, const char *format)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    ImGuiStorage* storage = window->DC.StateStorage;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    // =========================================================================
    // 스칼라 타입 변환 유틸리티 (void* <-> double)
    // =========================================================================
    auto GetAsDouble = [](ImGuiDataType type, const void* ptr) -> double {
        if (!ptr) return 0.0;
        switch (type) {
            case ImGuiDataType_S8:     return (double)*(const ImS8*)ptr;
            case ImGuiDataType_U8:     return (double)*(const ImU8*)ptr;
            case ImGuiDataType_S16:    return (double)*(const ImS16*)ptr;
            case ImGuiDataType_U16:    return (double)*(const ImU16*)ptr;
            case ImGuiDataType_S32:    return (double)*(const ImS32*)ptr;
            case ImGuiDataType_U32:    return (double)*(const ImU32*)ptr;
            case ImGuiDataType_S64:    return (double)*(const ImS64*)ptr;
            case ImGuiDataType_U64:    return (double)*(const ImU64*)ptr;
            case ImGuiDataType_Float:  return (double)*(const float*)ptr;
            case ImGuiDataType_Double: return *(const double*)ptr;
        }
        return 0.0;
    };

    auto SetFromDouble = [](ImGuiDataType type, void* ptr, double val) {
        if (!ptr) return;
        if (type >= ImGuiDataType_S8 && type <= ImGuiDataType_U64)
            val = std::round(val);

        switch (type) {
            case ImGuiDataType_S8:     *(ImS8*)ptr     = (ImS8)ImClamp(val, (double)-128.0, (double)127.0); break;
            case ImGuiDataType_U8:     *(ImU8*)ptr     = (ImU8)ImClamp(val, (double)0.0, (double)255.0); break;
            case ImGuiDataType_S16:    *(ImS16*)ptr    = (ImS16)ImClamp(val, (double)-32768.0, (double)32767.0); break;
            case ImGuiDataType_U16:    *(ImU16*)ptr    = (ImU16)ImClamp(val, (double)0.0, (double)65535.0); break;
            case ImGuiDataType_S32:    *(ImS32*)ptr    = (ImS32)ImClamp(val, (double)-2147483648.0, (double)2147483647.0); break;
            case ImGuiDataType_U32:    *(ImU32*)ptr    = (ImU32)ImClamp(val, (double)0.0, (double)4294967295.0); break;
            case ImGuiDataType_S64:    *(ImS64*)ptr    = (ImS64)val; break;
            case ImGuiDataType_U64:    *(ImU64*)ptr    = (ImU64)val; break;
            case ImGuiDataType_Float:  *(float*)ptr    = (float)val; break;
            case ImGuiDataType_Double: *(double*)ptr   = val; break;
        }
    };

    const ImGuiID text_mode_id = id + 2;
    const ImGuiID just_entered_id = id + 3;
    const ImGuiID focus_target_id = id + 4;

    bool text_input_mode = storage->GetBool(text_mode_id, false);

    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // ==========================================
    // [레이아웃 계산] 라벨은 왼쪽, 위젯은 우측 끝
    // ==========================================
    const float w = ImGui::CalcItemWidth();
    const float min_width = (label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f) + w;
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    const float frame_x = pos.x + total_width - w;
    const float frame_height = label_size.y + style.FramePadding.y * 2.0f;

    const ImRect frame_bb(ImVec2(frame_x, pos.y), ImVec2(frame_x + w, pos.y + frame_height));
    const ImRect total_bb(pos, ImVec2(pos.x + total_width, frame_bb.Max.y));

    // ==========================================
    // 1. 텍스트 입력 모드 (Ctrl + Click)
    // ==========================================
    if (text_input_mode)
    {
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id, &frame_bb, 0))
            return false;

        if (label_size.x > 0.0f)
        {
            const char* label_display_end = FindRenderedTextEnd(label);
            if (label != label_display_end)
                ImGui::RenderText(ImVec2(pos.x, pos.y + style.FramePadding.y), label, label_display_end);
        }

        ImGui::PushID(label);
        ImGui::SetCursorScreenPos(ImVec2(frame_x, pos.y));
        ImGui::PushMultiItemsWidths(2, w);

        bool just_entered = storage->GetBool(just_entered_id, false);
        if (just_entered)
        {
            int focus_idx = storage->GetInt(focus_target_id, 0);
            ImGui::SetKeyboardFocusHere(focus_idx);
            storage->SetBool(just_entered_id, false);
        }

        bool value_changed = false;
        value_changed |= ImGui::InputScalar("##min", data_type, p_min, NULL, NULL, format);
        ImGuiID min_id = window->GetID("##min");
        ImGui::PopItemWidth();
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        value_changed |= ImGui::InputScalar("##max", data_type, p_max, NULL, NULL, format);
        ImGuiID max_id = window->GetID("##max");
        ImGui::PopItemWidth();
        ImGui::PopID();

        if (value_changed)
        {
            double v_min_d = GetAsDouble(data_type, p_min);
            double v_max_d = GetAsDouble(data_type, p_max);
            double b_min_d = GetAsDouble(data_type, p_bound_min);
            double b_max_d = GetAsDouble(data_type, p_bound_max);

            v_min_d = ImClamp(v_min_d, b_min_d, b_max_d);
            v_max_d = ImClamp(v_max_d, b_min_d, b_max_d);

            if (v_min_d > v_max_d)
            {
                if (g.ActiveId == min_id) v_min_d = v_max_d;
                else if (g.ActiveId == max_id) v_max_d = v_min_d;
                else { double t = v_min_d; v_min_d = v_max_d; v_max_d = t; }
            }
            SetFromDouble(data_type, p_min, v_min_d);
            SetFromDouble(data_type, p_max, v_max_d);
        }

        if (!just_entered && g.ActiveId != min_id && g.ActiveId != max_id)
            storage->SetBool(text_mode_id, false);

        return value_changed;
    }

    // ==========================================
    // 2. 기본 슬라이더 모드 (드래그)
    // ==========================================
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
        return false;

    float grab_width = 12.0f;
    float track_height = 4.0f;
    float track_x0 = frame_bb.Min.x + grab_width * 0.5f;
    float track_x1 = frame_bb.Max.x - grab_width * 0.5f;
    float track_w = track_x1 - track_x0;

    double b_min_d = GetAsDouble(data_type, p_bound_min);
    double b_max_d = GetAsDouble(data_type, p_bound_max);

    auto ValToPosD = [&](double v) -> float {
        float t = (b_max_d == b_min_d) ? 0.0f : (float)((v - b_min_d) / (b_max_d - b_min_d));
        return track_x0 + ImClamp(t, 0.0f, 1.0f) * track_w;
    };

    double v_min_d = GetAsDouble(data_type, p_min);
    double v_max_d = GetAsDouble(data_type, p_max);

    float left_x = ValToPosD(v_min_d);
    float right_x = ValToPosD(v_max_d);

    bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool value_changed = false;

    int active_grab = storage->GetInt(id, 0);
    ImGuiID offset_id = id + 1;

    float hitbox_radius = grab_width * 0.5f + 4.0f;
    ImRect left_grab_bb(ImVec2(left_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(left_x + hitbox_radius, frame_bb.Max.y + 4.0f));
    ImRect right_grab_bb(ImVec2(right_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(right_x + hitbox_radius, frame_bb.Max.y + 4.0f));

    if (hovered && g.IO.MouseClicked[0])
    {
        bool left_hovered = left_grab_bb.Contains(g.IO.MousePos);
        bool right_hovered = right_grab_bb.Contains(g.IO.MousePos);

        if (g.IO.KeyCtrl)
        {
            storage->SetBool(text_mode_id, true);
            storage->SetBool(just_entered_id, true);
            int focus_idx = 0;
            if (right_hovered && !left_hovered) focus_idx = 1;
            else if (!left_hovered && !right_hovered) {
                focus_idx = (std::abs(g.IO.MousePos.x - right_x) < std::abs(g.IO.MousePos.x - left_x)) ? 1 : 0;
            }
            storage->SetInt(focus_target_id, focus_idx);
            ImGui::ClearActiveID();
            return false;
        }

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
            float grab_center_x = (active_grab == 1) ? left_x : right_x;
            storage->SetFloat(offset_id, g.IO.MousePos.x - grab_center_x);

            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }
    }

    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            float click_offset = storage->GetFloat(offset_id, 0.0f);
            float adjusted_mouse_x = g.IO.MousePos.x - click_offset;
            float t = ImClamp((adjusted_mouse_x - track_x0) / track_w, 0.0f, 1.0f);
            double v_new_d = b_min_d + t * (b_max_d - b_min_d);

            if (active_grab == 1) {
                SetFromDouble(data_type, p_min, ImMin(v_new_d, v_max_d));
                value_changed = true;
            }
            else if (active_grab == 2) {
                SetFromDouble(data_type, p_max, ImMax(v_new_d, v_min_d));
                value_changed = true;
            }

            v_min_d = GetAsDouble(data_type, p_min);
            v_max_d = GetAsDouble(data_type, p_max);
            left_x = ValToPosD(v_min_d);
            right_x = ValToPosD(v_max_d);
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
    // 3. 커스텀 렌더링 (슬라이더 바 및 삼각형 그랩)
    // ==========================================
    ImGui::RenderNavCursor(frame_bb, id);

    float track_y = std::floor(frame_bb.GetCenter().y + 0.5f);
    float lx = std::floor(left_x + 0.5f);
    float rx = std::floor(right_x + 0.5f);

    ImVec2 track_min = ImVec2(frame_bb.Min.x, track_y - track_height * 0.5f);
    ImVec2 track_max = ImVec2(frame_bb.Max.x, track_y + track_height * 0.5f);

    ImU32 bg_track_col = ImGui::GetColorU32(ImGuiCol_ScrollbarGrab);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    window->DrawList->AddRectFilled(ImVec2(lx, track_min.y), ImVec2(rx, track_max.y), fill_track_col, track_height * 0.5f);

    ImU32 grab_col = IM_COL32(255, 255, 255, 255);
    ImU32 grab_border_col = ImGui::GetColorU32(ImGuiCol_Border);
    float grab_border_thickness = 1.0f;
    float tri_w = 7.0f, tri_h = 9.0f, corner_radius = 2.0f;

    auto AddRoundedTriangle = [&](ImVec2 p1, ImVec2 p2, ImVec2 p3, float radius, ImU32 fill_col, ImU32 border_col, float border_thickness) {
        auto build_path = [&]() {
            ImVec2 pts[3] = { p1, p2, p3 };
            for (int i = 0; i < 3; i++) {
                ImVec2 prev = pts[(i + 2) % 3], curr = pts[i], next = pts[(i + 1) % 3];
                ImVec2 v1(prev.x - curr.x, prev.y - curr.y), v2(next.x - curr.x, next.y - curr.y);
                float len1 = std::sqrt(v1.x * v1.x + v1.y * v1.y), len2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
                if (len1 > 0.0f) { v1.x /= len1; v1.y /= len1; }
                if (len2 > 0.0f) { v2.x /= len2; v2.y /= len2; }
                float angle1 = std::atan2(v1.y, v1.x), angle2 = std::atan2(v2.y, v2.x);
                float diff = angle2 - angle1;
                while (diff < -IM_PI) diff += IM_PI * 2.0f;
                while (diff > IM_PI) diff -= IM_PI * 2.0f;
                window->DrawList->PathArcTo(curr, radius, angle1, angle1 + diff, 6);
            }
        };
        build_path(); window->DrawList->PathFillConvex(fill_col);
        if (border_thickness > 0.0f) {
            build_path(); window->DrawList->PathStroke(border_col, ImDrawFlags_Closed, border_thickness);
        }
    };

    AddRoundedTriangle(ImVec2(lx - tri_w, track_y - tri_h), ImVec2(lx + tri_w, track_y), ImVec2(lx - tri_w, track_y + tri_h), corner_radius, grab_col, grab_border_col, grab_border_thickness);
    AddRoundedTriangle(ImVec2(rx + tri_w, track_y - tri_h), ImVec2(rx + tri_w, track_y + tri_h), ImVec2(rx - tri_w, track_y), corner_radius, grab_col, grab_border_col, grab_border_thickness);

    // ==========================================
    // 4. 라벨 표시 (가장 좌측) 및 커스텀 말풍선 툴팁
    // ==========================================

    // 라벨 렌더링
    if (label_size.x > 0.0f) {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
            ImGui::RenderText(ImVec2(pos.x, frame_bb.Min.y + style.FramePadding.y), label, label_display_end);
    }

    // 드래그(Active) 중일 때 디자인된 말풍선 출력
    if (g.ActiveId == id)
    {
        char val_min_buf[64], val_max_buf[64];
        DataTypeFormatString(val_min_buf, sizeof(val_min_buf), data_type, p_min, format);
        DataTypeFormatString(val_max_buf, sizeof(val_max_buf), data_type, p_max, format);

        #ifndef ICON_MD_ARROW_RANGE
        #define ICON_MD_ARROW_RANGE "-"
        #endif

        char value_buf[128];
        snprintf(value_buf, sizeof(value_buf), "%s " ICON_MD_ARROW_RANGE " %s", val_min_buf, val_max_buf);

        // 현재 조작 중인 그랩을 기준으로 위치 설정 (중앙 혹은 조작중인 위치)
        float active_x = (left_x + right_x) * 0.5f;
        if (active_grab == 1) active_x = left_x;
        else if (active_grab == 2) active_x = right_x;

        // 툴팁 렌더링을 윈도우 영역 밖에서도 잘리지 않도록 최상단 Foreground DrawList 사용
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        ImVec2 text_size = ImGui::CalcTextSize(value_buf);
        ImVec2 padding(10.0f, 6.0f);
        float tooltip_y_offset = 12.0f;

        // 삼각형 그랩의 윗변(track_y - tri_h)을 기준으로 말풍선 위치 계산
        float grab_top_y = track_y - tri_h;

        ImVec2 tooltip_min = ImVec2(active_x - text_size.x * 0.5f - padding.x, grab_top_y - tooltip_y_offset - text_size.y - padding.y * 2.0f);
        ImVec2 tooltip_max = ImVec2(active_x + text_size.x * 0.5f + padding.x, grab_top_y - tooltip_y_offset);

        ImU32 tooltip_bg_col     = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 tooltip_border_col = ImGui::GetColorU32(ImGuiCol_Border);

        // 1. 말풍선 둥근 사각형 배경 & 테두리
        draw_list->AddRectFilled(tooltip_min, tooltip_max, tooltip_bg_col, 6.0f);
        draw_list->AddRect(tooltip_min, tooltip_max, tooltip_border_col, 6.0f);

        // 꼬리 꼭짓점 기본 좌표 (사각형 하단 선 기준)
        ImVec2 p1 = ImVec2(active_x - 6.0f, tooltip_max.y);
        ImVec2 p2 = ImVec2(active_x + 6.0f, tooltip_max.y);
        ImVec2 p3 = ImVec2(active_x, tooltip_max.y + 6.0f); // 꼬리 끝(아래)

        // 2. 몸통과 꼬리가 만나는 부분의 테두리를 확실하게 지우기
        draw_list->AddRectFilled(
            ImVec2(p1.x + 0.5f, tooltip_max.y - 1.0f),
            ImVec2(p2.x - 0.5f, tooltip_max.y + 1.0f),
            tooltip_bg_col
        );

        // 3. 꼬리 배경 삼각형 그리기
        ImVec2 fill_p1 = ImVec2(p1.x, tooltip_max.y - 1.0f);
        ImVec2 fill_p2 = ImVec2(p2.x, tooltip_max.y - 1.0f);
        draw_list->AddTriangleFilled(fill_p1, fill_p2, p3, tooltip_bg_col);

        // 4. 꼬리 테두리 (V자 선)
        ImVec2 tail_pts[3] = { p1, p3, p2 };
        draw_list->AddPolyline(tail_pts, 3, tooltip_border_col, 0, 1.0f);

        // 5. 텍스트 렌더링
        draw_list->AddText(tooltip_min + padding, ImGui::GetColorU32(ImGuiCol_Text), value_buf);
    }

    return value_changed;
}


bool ImGui::_slider1_(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();
    const ImU32 color_marker = (g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasColorMarker) ? g.NextItemData.ColorMarker : 0;

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 pos = window->DC.CursorPos;

    // [수정] 전체 가용 영역 및 슬라이더 위젯이 배치될 오른쪽 좌표 계산
    const float min_width = (label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f) + w;
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    // 슬라이더 위젯을 오른쪽 끝으로 배치
    const float frame_x = pos.x + total_width - w;
    const ImRect frame_bb(ImVec2(frame_x, pos.y), ImVec2(frame_x + w, pos.y + label_size.y + style.FramePadding.y * 2.0f));

    // 전체 차지 영역 (라벨부터 슬라이더 끝까지)
    const ImRect total_bb(pos, ImVec2(pos.x + total_width, frame_bb.Max.y));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Slider turns it into an input box
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp Ctrl+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, false, style.FrameRounding);
    if (color_marker != 0 && style.ColorMarkerSize > 0.0f)
        RenderColorComponentMarker(frame_bb, GetColorU32(color_marker), style.FrameRounding);
    RenderFrameBorder(frame_bb.Min, frame_bb.Max, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_COUNTOF(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    // [수정] 라벨(텍스트)을 가장 왼쪽 위치(pos.x)에 렌더링
    if (label_size.x > 0.0f)
        RenderText(ImVec2(pos.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ImGui::SliderFloatX(const char *label, float *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider1_(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderFloatX(const char *label, int *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider1_(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderFloatX(const char *label, double *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider1_(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

bool ImGui::Slider(const char *label, float *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider2_(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool ImGui::Slider(const char *label, int *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider2_(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool ImGui::_slider2_(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    // [수정] 슬라이더 위젯의 너비 계산 (기본 아이템 너비 사용)
    const float w = CalcItemWidth();

    // [수정] 전체 가용 영역 및 위젯이 배치될 오른쪽 좌표 계산
    const float min_width = (label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f) + w;
    const float total_width = ImMax(min_width, window->WorkRect.Max.x - pos.x);

    // 슬라이더 프레임을 오른쪽 끝에 배치
    const float frame_x = pos.x + total_width - w;
    const float frame_height = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect frame_bb(ImVec2(frame_x, pos.y), ImVec2(frame_x + w, pos.y + frame_height));

    // 전체 차지 영역 (라벨부터 위젯 끝까지)
    const ImRect total_bb(pos, ImVec2(pos.x + total_width, frame_bb.Max.y));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    ImRect slider_bb = frame_bb;
    slider_bb.Min.x += 5.0f;
    slider_bb.Max.x -= 5.0f;

    // =========================================================================
    // 1. 그랩 밖 클릭 무시를 위한 히트박스 계산 (비율 계산용 double 변환)
    // =========================================================================
    auto GetAsDouble = [](ImGuiDataType type, const void* ptr) -> double {
        if (!ptr) return 0.0;
        switch (type) {
            case ImGuiDataType_S8:     return (double)*(const ImS8*)ptr;
            case ImGuiDataType_U8:     return (double)*(const ImU8*)ptr;
            case ImGuiDataType_S16:    return (double)*(const ImS16*)ptr;
            case ImGuiDataType_U16:    return (double)*(const ImU16*)ptr;
            case ImGuiDataType_S32:    return (double)*(const ImS32*)ptr;
            case ImGuiDataType_U32:    return (double)*(const ImU32*)ptr;
            case ImGuiDataType_S64:    return (double)*(const ImS64*)ptr;
            case ImGuiDataType_U64:    return (double)*(const ImU64*)ptr;
            case ImGuiDataType_Float:  return (double)*(const float*)ptr;
            case ImGuiDataType_Double: return *(const double*)ptr;
        }
        return 0.0;
    };

    double v_val = GetAsDouble(data_type, p_data);
    double v_min_val = GetAsDouble(data_type, p_min);
    double v_max_val = GetAsDouble(data_type, p_max);

    double v_clamped = (v_min_val < v_max_val) ? ImClamp(v_val, v_min_val, v_max_val) : ImClamp(v_val, v_max_val, v_min_val);
    float t = 0.0f;
    if (v_min_val != v_max_val)
        t = (float)ImClamp((v_clamped - v_min_val) / (v_max_val - v_min_val), 0.0, 1.0);

    float grab_padding = 2.0f;
    float grab_radius = 9.0f;
    float grab_w = grab_radius * 2.0f;
    float internal_min_x = slider_bb.Min.x + grab_padding + grab_w * 0.5f;
    float internal_max_x = slider_bb.Max.x - grab_padding - grab_w * 0.5f;

    float grab_center_x = ImLerp(internal_min_x, internal_max_x, t);
    ImVec2 current_grab_center = ImVec2(grab_center_x, frame_bb.GetCenter().y);

    float grab_hitbox_radius = grab_radius + 4.0f;
    ImRect interact_grab_bb(
        current_grab_center.x - grab_hitbox_radius, frame_bb.Min.y - 4.0f,
        current_grab_center.x + grab_hitbox_radius, frame_bb.Max.y + 4.0f
    );

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool is_clicking_outside_grab = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id) && !interact_grab_bb.Contains(g.IO.MousePos);

    // =========================================================================
    // 2. 텍스트 인풋 모드 (Ctrl + Click) 처리 로직
    // =========================================================================
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);

        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // =========================================================================
    // 3. 실제 슬라이더 동작 및 클릭 제한 적용
    // =========================================================================
    bool backup_clicked = g.IO.MouseClicked[0];
    bool backup_down = g.IO.MouseDown[0];

    if (is_clicking_outside_grab)
    {
        g.IO.MouseClicked[0] = false;
        g.IO.MouseDown[0] = false;
    }

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(slider_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);

    if (is_clicking_outside_grab)
    {
        g.IO.MouseClicked[0] = backup_clicked;
        g.IO.MouseDown[0] = backup_down;
    }

    if (value_changed)
        MarkItemEdited(id);

    // ==========================================
    // 4. 이미지 스타일 커스텀 렌더링
    // ==========================================
    RenderNavCursor(frame_bb, id);

    auto GetMappedGrabCenter = [&](const ImRect& g_bb) -> ImVec2 {
        float g_w = g_bb.GetWidth();
        float internal_min_c = slider_bb.Min.x + grab_padding + g_w * 0.5f;
        float internal_max_c = slider_bb.Max.x - grab_padding - g_w * 0.5f;
        float t_render = 0.0f;
        if (internal_max_c > internal_min_c)
            t_render = ImClamp((g_bb.GetCenter().x - internal_min_c) / (internal_max_c - internal_min_c), 0.0f, 1.0f);
        float mapped_x = ImLerp(slider_bb.Min.x, slider_bb.Max.x, t_render);
        return ImVec2(mapped_x, frame_bb.GetCenter().y);
    };

    float track_height = 4.0f;
    ImVec2 track_min = ImVec2(frame_bb.Min.x, frame_bb.GetCenter().y - track_height * 0.5f);
    ImVec2 track_max = ImVec2(frame_bb.Max.x, frame_bb.GetCenter().y + track_height * 0.5f);

    ImU32 bg_track_col = GetColorU32(ImGuiCol_ScrollbarGrab);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImVec2 grab_center = GetMappedGrabCenter(grab_bb);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    if (grab_center.x > frame_bb.Min.x)
    {
        ImVec2 fill_max = ImVec2(grab_center.x, track_max.y);
        window->DrawList->AddRectFilled(track_min, fill_max, fill_track_col, track_height * 0.5f);
    }

    window->DrawList->AddCircleFilled(grab_center, grab_radius, IM_COL32(255, 255, 255, 255));
    ImU32 grab_border_col = GetColorU32(ImGuiCol_Border);
    float grab_border_thickness = 1.0f;
    window->DrawList->AddCircle(grab_center, grab_radius, grab_border_col, 0, grab_border_thickness);

    // ==========================================
    // 5. 드래그 중 값 툴팁 표시 및 라벨 렌더링
    // ==========================================

    // 포맷팅된 값을 담을 버퍼
    char value_buf[64];
    DataTypeFormatString(value_buf, IM_COUNTOF(value_buf), data_type, p_data, format);

    // [수정] 위젯과 상호작용 중일 때(마우스 누르기/드래그) 툴팁으로만 값 출력
    if (g.ActiveId == id)
    {
        ImVec2 text_size = CalcTextSize(value_buf);
        ImVec2 padding(10.0f, 6.0f);
        float tooltip_y_offset = 12.0f;

        ImVec2 tooltip_min = ImVec2(grab_center.x - text_size.x * 0.5f - padding.x, grab_center.y - grab_radius - tooltip_y_offset - text_size.y - padding.y * 2.0f);
        ImVec2 tooltip_max = ImVec2(grab_center.x + text_size.x * 0.5f + padding.x, grab_center.y - grab_radius - tooltip_y_offset);

        ImU32 tooltip_bg_col     = GetColorU32(ImGuiCol_FrameBg);
        ImU32 tooltip_border_col = GetColorU32(ImGuiCol_Border);

        // 1. 말풍선 둥근 사각형 배경 & 테두리
        window->DrawList->AddRectFilled(tooltip_min, tooltip_max, tooltip_bg_col, 6.0f);
        window->DrawList->AddRect(tooltip_min, tooltip_max, tooltip_border_col, 6.0f);

        // 꼬리 꼭짓점 기본 좌표 (사각형 하단 선 기준)
        ImVec2 p1 = ImVec2(grab_center.x - 6.0f, tooltip_max.y);
        ImVec2 p2 = ImVec2(grab_center.x + 6.0f, tooltip_max.y);
        ImVec2 p3 = ImVec2(grab_center.x, tooltip_max.y + 6.0f); // 꼬리 끝(아래)

        // 2. 몸통과 꼬리가 만나는 부분의 테두리를 확실하게 지우기
        // 높이 2px짜리 배경색 사각형을 테두리 위에 덮어씌워 잔상을 없앱니다.
        // 양끝 모서리가 잘리지 않도록 0.5f씩 좁혀서 덮어줍니다.
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
        // 테두리 선이 사각형 바닥선(tooltip_max.y)에서 딱 떨어지게 연결됩니다.
        ImVec2 tail_pts[3] = { p1, p3, p2 };
        window->DrawList->AddPolyline(tail_pts, 3, tooltip_border_col, 0, 1.0f);

        // 5. 텍스트 렌더링
        window->DrawList->AddText(tooltip_min + padding, GetColorU32(ImGuiCol_Text), value_buf);
    }

    // [수정] 라벨(텍스트)을 가장 왼쪽 위치(pos.x)에 렌더링
    if (label_size.x > 0.0f)
    {
        RenderText(ImVec2(pos.x, frame_bb.Min.y + style.FramePadding.y), label);
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ImGui::Slider(const char *label, double *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider2_(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderX(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return _slider3(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderX(const char *label, int *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider3(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool ImGui::SliderX(const char *label, double *v, float v_min, float v_max, const char *format, ImGuiSliderFlags flags)
{
    return _slider3(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

bool ImGui::_slider3(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const float w = ImMax(10.0f, window->WorkRect.Max.x - pos.x);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float text_height = g.FontSize;
    const float spacing = style.ItemInnerSpacing.y;

    const ImRect text_bb(pos, pos + ImVec2(w, text_height));
    const float frame_height = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect frame_bb(ImVec2(pos.x, pos.y + text_height + spacing),
                          ImVec2(pos.x + w, pos.y + text_height + spacing + frame_height));
    const ImRect total_bb(pos, frame_bb.Max);

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    ImRect slider_bb = frame_bb;
    slider_bb.Min.x += 5.0f;
    slider_bb.Max.x -= 5.0f;

    // =========================================================================
    // 1. 그랩 밖 클릭 무시를 위한 히트박스 수학적 계산
    // DataType 제네릭 처리를 위해 double로 캐스팅하여 비율(t) 계산
    // =========================================================================
    auto GetAsDouble = [](ImGuiDataType type, const void* ptr) -> double {
        if (!ptr) return 0.0;
        switch (type) {
            case ImGuiDataType_S8:     return (double)*(const ImS8*)ptr;
            case ImGuiDataType_U8:     return (double)*(const ImU8*)ptr;
            case ImGuiDataType_S16:    return (double)*(const ImS16*)ptr;
            case ImGuiDataType_U16:    return (double)*(const ImU16*)ptr;
            case ImGuiDataType_S32:    return (double)*(const ImS32*)ptr;
            case ImGuiDataType_U32:    return (double)*(const ImU32*)ptr;
            case ImGuiDataType_S64:    return (double)*(const ImS64*)ptr;
            case ImGuiDataType_U64:    return (double)*(const ImU64*)ptr;
            case ImGuiDataType_Float:  return (double)*(const float*)ptr;
            case ImGuiDataType_Double: return *(const double*)ptr;
        }
        return 0.0;
    };

    double v_val = GetAsDouble(data_type, p_data);
    double v_min_val = GetAsDouble(data_type, p_min);
    double v_max_val = GetAsDouble(data_type, p_max);

    double v_clamped = (v_min_val < v_max_val) ? ImClamp(v_val, v_min_val, v_max_val) : ImClamp(v_val, v_max_val, v_min_val);
    float t = 0.0f;
    if (v_min_val != v_max_val)
        t = (float)ImClamp((v_clamped - v_min_val) / (v_max_val - v_min_val), 0.0, 1.0);

    float grab_padding = 2.0f;
    float grab_radius = 9.0f;
    float grab_w = grab_radius * 2.0f;
    float internal_min_x = slider_bb.Min.x + grab_padding + grab_w * 0.5f;
    float internal_max_x = slider_bb.Max.x - grab_padding - grab_w * 0.5f;

    float grab_center_x = ImLerp(internal_min_x, internal_max_x, t);
    ImVec2 current_grab_center = ImVec2(grab_center_x, frame_bb.GetCenter().y);

    float grab_hitbox_radius = grab_radius + 4.0f;
    ImRect interact_grab_bb(
        current_grab_center.x - grab_hitbox_radius, frame_bb.Min.y - 4.0f,
        current_grab_center.x + grab_hitbox_radius, frame_bb.Max.y + 4.0f
    );

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool is_clicking_outside_grab = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id) && !interact_grab_bb.Contains(g.IO.MousePos);

    // =========================================================================
    // 2. 텍스트 인풋 모드 (Ctrl + Click) 처리 로직
    // =========================================================================
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);

        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // =========================================================================
    // 3. 실제 슬라이더 동작 및 조작 영역 클릭 제한 적용
    // =========================================================================
    bool backup_clicked = g.IO.MouseClicked[0];
    bool backup_down = g.IO.MouseDown[0];

    if (is_clicking_outside_grab)
    {
        g.IO.MouseClicked[0] = false;
        g.IO.MouseDown[0] = false;
    }

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(slider_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);

    if (is_clicking_outside_grab)
    {
        g.IO.MouseClicked[0] = backup_clicked;
        g.IO.MouseDown[0] = backup_down;
    }

    if (value_changed)
        MarkItemEdited(id);

    // ==========================================
    // --- 이미지 스타일 커스텀 렌더링 시작 ---
    // ==========================================
    RenderNavCursor(frame_bb, id);

    auto GetMappedGrabCenter = [&](const ImRect& g_bb) -> ImVec2 {
        float g_w = g_bb.GetWidth();
        float internal_min_c = slider_bb.Min.x + grab_padding + g_w * 0.5f;
        float internal_max_c = slider_bb.Max.x - grab_padding - g_w * 0.5f;
        float t_render = 0.0f;
        if (internal_max_c > internal_min_c)
            t_render = ImClamp((g_bb.GetCenter().x - internal_min_c) / (internal_max_c - internal_min_c), 0.0f, 1.0f);
        float mapped_x = ImLerp(slider_bb.Min.x, slider_bb.Max.x, t_render);
        return ImVec2(mapped_x, frame_bb.GetCenter().y);
    };

    float track_height = 4.0f;
    ImVec2 track_min = ImVec2(frame_bb.Min.x, frame_bb.GetCenter().y - track_height * 0.5f);
    ImVec2 track_max = ImVec2(frame_bb.Max.x, frame_bb.GetCenter().y + track_height * 0.5f);

    ImU32 bg_track_col = GetColorU32(ImGuiCol_ScrollbarGrab);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImVec2 grab_center = GetMappedGrabCenter(grab_bb);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    if (grab_center.x > frame_bb.Min.x)
    {
        ImVec2 fill_max = ImVec2(grab_center.x, track_max.y);
        window->DrawList->AddRectFilled(track_min, fill_max, fill_track_col, track_height * 0.5f);
    }

    window->DrawList->AddCircleFilled(grab_center, grab_radius, IM_COL32(255, 255, 255, 255));
    ImU32 grab_border_col = GetColorU32(ImGuiCol_Border);
    float grab_border_thickness = 1.0f;
    window->DrawList->AddCircle(grab_center, grab_radius, grab_border_col, 0, grab_border_thickness);

    // 데이터 타입에 맞는 텍스트 포맷 처리
    char value_buf[64];
    int value_len = DataTypeFormatString(value_buf, IM_COUNTOF(value_buf), data_type, p_data, format);
    const char* value_buf_end = value_buf + value_len;

    // --- 텍스트 렌더링 로직 ---
    if (label_size.x > 0.0f)
    {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
            RenderText(text_bb.Min, label, label_display_end);
    }

    const ImVec2 value_size = CalcTextSize(value_buf, value_buf_end);
    RenderText(ImVec2(text_bb.Max.x - value_size.x, text_bb.Min.y), value_buf, value_buf_end);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ImGui::_slider5_(const char *label, ImGuiDataType data_type, void *p_min, void *p_max, const void *p_bound_min, const void *p_bound_max, const char *format)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    ImGuiStorage* storage = window->DC.StateStorage;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    // =========================================================================
    // 스칼라 타입 변환 유틸리티 (void* <-> double)
    // =========================================================================
    auto GetAsDouble = [](ImGuiDataType type, const void* ptr) -> double {
        if (!ptr) return 0.0;
        switch (type) {
            case ImGuiDataType_S8:     return (double)*(const ImS8*)ptr;
            case ImGuiDataType_U8:     return (double)*(const ImU8*)ptr;
            case ImGuiDataType_S16:    return (double)*(const ImS16*)ptr;
            case ImGuiDataType_U16:    return (double)*(const ImU16*)ptr;
            case ImGuiDataType_S32:    return (double)*(const ImS32*)ptr;
            case ImGuiDataType_U32:    return (double)*(const ImU32*)ptr;
            case ImGuiDataType_S64:    return (double)*(const ImS64*)ptr;
            case ImGuiDataType_U64:    return (double)*(const ImU64*)ptr;
            case ImGuiDataType_Float:  return (double)*(const float*)ptr;
            case ImGuiDataType_Double: return *(const double*)ptr;
        }
        return 0.0;
    };

    auto SetFromDouble = [](ImGuiDataType type, void* ptr, double val) {
        if (!ptr) return;
        // 정수형 타입일 경우 반올림 처리
        if (type >= ImGuiDataType_S8 && type <= ImGuiDataType_U64)
            val = std::round(val);

        switch (type) {
            case ImGuiDataType_S8:     *(ImS8*)ptr     = (ImS8)ImClamp(val, (double)-128.0, (double)127.0); break;
            case ImGuiDataType_U8:     *(ImU8*)ptr     = (ImU8)ImClamp(val, (double)0.0, (double)255.0); break;
            case ImGuiDataType_S16:    *(ImS16*)ptr    = (ImS16)ImClamp(val, (double)-32768.0, (double)32767.0); break;
            case ImGuiDataType_U16:    *(ImU16*)ptr    = (ImU16)ImClamp(val, (double)0.0, (double)65535.0); break;
            case ImGuiDataType_S32:    *(ImS32*)ptr    = (ImS32)ImClamp(val, (double)-2147483648.0, (double)2147483647.0); break;
            case ImGuiDataType_U32:    *(ImU32*)ptr    = (ImU32)ImClamp(val, (double)0.0, (double)4294967295.0); break;
            case ImGuiDataType_S64:    *(ImS64*)ptr    = (ImS64)val; break;
            case ImGuiDataType_U64:    *(ImU64*)ptr    = (ImU64)val; break;
            case ImGuiDataType_Float:  *(float*)ptr    = (float)val; break;
            case ImGuiDataType_Double: *(double*)ptr   = val; break;
        }
    };

    // 텍스트 모드 전환 상태를 저장하기 위한 커스텀 ID
    const ImGuiID text_mode_id = id + 2;
    const ImGuiID just_entered_id = id + 3;
    const ImGuiID focus_target_id = id + 4;

    bool text_input_mode = storage->GetBool(text_mode_id, false);

    // [수정] 시작 좌표를 먼저 가져오고, 우측 여백 없이 끝까지 너비(w)를 계산
    const ImVec2 pos = window->DC.CursorPos;
    const float w = ImMax(10.0f, window->WorkRect.Max.x - pos.x);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // ==========================================
    // 텍스트 입력 모드 렌더링 (컨트롤 클릭 시 표시됨)
    // ==========================================
    if (text_input_mode)
    {
        ImGui::BeginGroup();

        if (label_size.x > 0.0f)
        {
            const char* label_display_end = FindRenderedTextEnd(label);
            if (label != label_display_end)
                ImGui::TextEx(label, label_display_end);
        }

        ImGui::PushID(label);

        // 변경된 가용 전체 너비(w)를 기반으로 2등분
        ImGui::PushMultiItemsWidths(2, w);

        bool just_entered = storage->GetBool(just_entered_id, false);
        if (just_entered)
        {
            int focus_idx = storage->GetInt(focus_target_id, 0);
            ImGui::SetKeyboardFocusHere(focus_idx);
            storage->SetBool(just_entered_id, false);
        }

        bool value_changed = false;

        value_changed |= ImGui::InputScalar("##min", data_type, p_min, NULL, NULL, format);
        ImGuiID min_id = window->GetID("##min");
        ImGui::PopItemWidth();
        ImGui::SameLine(0, style.ItemInnerSpacing.x);

        value_changed |= ImGui::InputScalar("##max", data_type, p_max, NULL, NULL, format);
        ImGuiID max_id = window->GetID("##max");
        ImGui::PopItemWidth();

        ImGui::PopID();
        ImGui::EndGroup();

        if (value_changed)
        {
            double v_min_d = GetAsDouble(data_type, p_min);
            double v_max_d = GetAsDouble(data_type, p_max);
            double b_min_d = GetAsDouble(data_type, p_bound_min);
            double b_max_d = GetAsDouble(data_type, p_bound_max);

            v_min_d = ImClamp(v_min_d, b_min_d, b_max_d);
            v_max_d = ImClamp(v_max_d, b_min_d, b_max_d);

            if (v_min_d > v_max_d)
            {
                if (g.ActiveId == min_id) v_min_d = v_max_d;
                else if (g.ActiveId == max_id) v_max_d = v_min_d;
                else { double t = v_min_d; v_min_d = v_max_d; v_max_d = t; }
            }
            SetFromDouble(data_type, p_min, v_min_d);
            SetFromDouble(data_type, p_max, v_max_d);
        }

        if (!just_entered && g.ActiveId != min_id && g.ActiveId != max_id)
        {
            storage->SetBool(text_mode_id, false);
        }

        return value_changed;
    }

    // ==========================================
    // 이하 슬라이더 바(기본) 모드 로직
    // ==========================================
    const float text_height = g.FontSize;
    const float spacing = style.ItemInnerSpacing.y;

    const ImRect text_bb(pos, pos + ImVec2(w, text_height));
    const float frame_height = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect frame_bb(ImVec2(pos.x, pos.y + text_height + spacing),
                          ImVec2(pos.x + w, pos.y + text_height + spacing + frame_height));
    const ImRect total_bb(pos, frame_bb.Max);

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
        return false;

    float grab_width = 12.0f;
    float track_height = 4.0f;

    float track_x0 = frame_bb.Min.x + grab_width * 0.5f;
    float track_x1 = frame_bb.Max.x - grab_width * 0.5f;
    float track_w = track_x1 - track_x0;

    double b_min_d = GetAsDouble(data_type, p_bound_min);
    double b_max_d = GetAsDouble(data_type, p_bound_max);

    auto ValToPosD = [&](double v) -> float {
        float t = (b_max_d == b_min_d) ? 0.0f : (float)((v - b_min_d) / (b_max_d - b_min_d));
        return track_x0 + ImClamp(t, 0.0f, 1.0f) * track_w;
    };

    double v_min_d = GetAsDouble(data_type, p_min);
    double v_max_d = GetAsDouble(data_type, p_max);

    float left_x = ValToPosD(v_min_d);
    float right_x = ValToPosD(v_max_d);

    bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool value_changed = false;

    int active_grab = storage->GetInt(id, 0);
    ImGuiID offset_id = id + 1;

    float hitbox_radius = grab_width * 0.5f + 4.0f;
    ImRect left_grab_bb(ImVec2(left_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(left_x + hitbox_radius, frame_bb.Max.y + 4.0f));
    ImRect right_grab_bb(ImVec2(right_x - hitbox_radius, frame_bb.Min.y - 4.0f), ImVec2(right_x + hitbox_radius, frame_bb.Max.y + 4.0f));

    if (hovered && g.IO.MouseClicked[0])
    {
        bool left_hovered = left_grab_bb.Contains(g.IO.MousePos);
        bool right_hovered = right_grab_bb.Contains(g.IO.MousePos);

        if (g.IO.KeyCtrl)
        {
            storage->SetBool(text_mode_id, true);
            storage->SetBool(just_entered_id, true);

            int focus_idx = 0;
            if (right_hovered && !left_hovered) focus_idx = 1;
            else if (!left_hovered && !right_hovered) {
                focus_idx = (std::abs(g.IO.MousePos.x - right_x) < std::abs(g.IO.MousePos.x - left_x)) ? 1 : 0;
            }
            storage->SetInt(focus_target_id, focus_idx);

            ImGui::ClearActiveID();
            return false;
        }

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
            float grab_center_x = (active_grab == 1) ? left_x : right_x;
            storage->SetFloat(offset_id, g.IO.MousePos.x - grab_center_x);

            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }
    }

    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            float click_offset = storage->GetFloat(offset_id, 0.0f);
            float adjusted_mouse_x = g.IO.MousePos.x - click_offset;
            float t = ImClamp((adjusted_mouse_x - track_x0) / track_w, 0.0f, 1.0f);
            double v_new_d = b_min_d + t * (b_max_d - b_min_d);

            if (active_grab == 1)
            {
                SetFromDouble(data_type, p_min, ImMin(v_new_d, v_max_d));
                value_changed = true;
            }
            else if (active_grab == 2)
            {
                SetFromDouble(data_type, p_max, ImMax(v_new_d, v_min_d));
                value_changed = true;
            }

            v_min_d = GetAsDouble(data_type, p_min);
            v_max_d = GetAsDouble(data_type, p_max);
            left_x = ValToPosD(v_min_d);
            right_x = ValToPosD(v_max_d);
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
    // 커스텀 렌더링 로직 (삼각형 렌더링 등)
    // ==========================================
    float track_y = std::floor(frame_bb.GetCenter().y + 0.5f);
    float lx = std::floor(left_x + 0.5f);
    float rx = std::floor(right_x + 0.5f);

    ImVec2 track_min = ImVec2(frame_bb.Min.x, track_y - track_height * 0.5f);
    ImVec2 track_max = ImVec2(frame_bb.Max.x, track_y + track_height * 0.5f);

    ImU32 bg_track_col = GetColorU32(ImGuiCol_ScrollbarGrab);
    window->DrawList->AddRectFilled(track_min, track_max, bg_track_col, track_height * 0.5f);

    ImU32 fill_track_col = ImGui::GetColorU32(ImVec4(0.3647f, 0.4117f, 0.9411f, 1.0f));
    window->DrawList->AddRectFilled(ImVec2(lx, track_min.y), ImVec2(rx, track_max.y), fill_track_col, track_height * 0.5f);

    ImU32 grab_col = IM_COL32(255, 255, 255, 255);
    ImU32 grab_border_col = GetColorU32(ImGuiCol_Border);
    float grab_border_thickness = 1.0f;
    float tri_w = 7.0f, tri_h = 9.0f, corner_radius = 2.0f;

    auto AddRoundedTriangle = [](ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImVec2 p3, float radius, ImU32 fill_col, ImU32 border_col, float border_thickness) {
        auto build_path = [&]() {
            ImVec2 pts[3] = { p1, p2, p3 };
            for (int i = 0; i < 3; i++) {
                ImVec2 prev = pts[(i + 2) % 3], curr = pts[i], next = pts[(i + 1) % 3];
                ImVec2 v1(prev.x - curr.x, prev.y - curr.y), v2(next.x - curr.x, next.y - curr.y);
                float len1 = std::sqrt(v1.x * v1.x + v1.y * v1.y), len2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);
                if (len1 > 0.0f) { v1.x /= len1; v1.y /= len1; }
                if (len2 > 0.0f) { v2.x /= len2; v2.y /= len2; }
                float angle1 = std::atan2(v1.y, v1.x), angle2 = std::atan2(v2.y, v2.x);
                float diff = angle2 - angle1;
                while (diff < -IM_PI) diff += IM_PI * 2.0f;
                while (diff > IM_PI) diff -= IM_PI * 2.0f;
                draw_list->PathArcTo(curr, radius, angle1, angle1 + diff, 6);
            }
        };
        build_path(); draw_list->PathFillConvex(fill_col);
        if (border_thickness > 0.0f) {
            build_path(); draw_list->PathStroke(border_col, ImDrawFlags_Closed, border_thickness);
        }
    };

    AddRoundedTriangle(window->DrawList, ImVec2(lx - tri_w, track_y - tri_h), ImVec2(lx + tri_w, track_y), ImVec2(lx - tri_w, track_y + tri_h), corner_radius, grab_col, grab_border_col, grab_border_thickness);
    AddRoundedTriangle(window->DrawList, ImVec2(rx + tri_w, track_y - tri_h), ImVec2(rx + tri_w, track_y + tri_h), ImVec2(rx - tri_w, track_y), corner_radius, grab_col, grab_border_col, grab_border_thickness);

    // ==========================================
    // --- 상단 텍스트 렌더링 로직 (라벨 및 값) ---
    // ==========================================
    if (label_size.x > 0.0f) {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
            ImGui::RenderText(text_bb.Min, label, label_display_end);
    }

    char val_min_buf[64], val_max_buf[64];
    DataTypeFormatString(val_min_buf, sizeof(val_min_buf), data_type, p_min, format);
    DataTypeFormatString(val_max_buf, sizeof(val_max_buf), data_type, p_max, format);

    // [수정] 텍스트가 가용 폭(w)의 우측 끝에 정확히 정렬됨
    char value_buf[128];
    snprintf(value_buf, sizeof(value_buf), "%s " ICON_MD_ARROW_RANGE " %s", val_min_buf, val_max_buf);

    const ImVec2 value_size = ImGui::CalcTextSize(value_buf);
    ImGui::RenderText(ImVec2(text_bb.Max.x - value_size.x, text_bb.Min.y), value_buf);

    return value_changed;
}

bool ImGui::SliderRangeX(const char *label, float *v_min, float *v_max, float v_bound_min, float v_bound_max, const char *format)
{
    return _slider5_(label, ImGuiDataType_Float, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::SliderRangeX(const char *label, int *v_min, int *v_max, float v_bound_min, float v_bound_max, const char *format)
{
    return _slider5_(label, ImGuiDataType_S32, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::SliderRangeX(const char *label, double *v_min, double *v_max, float v_bound_min, float v_bound_max, const char *format)
{
    return _slider5_(label, ImGuiDataType_Double, v_min, v_max, &v_bound_min, &v_bound_max, format);
}

bool ImGui::Toggle(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // 1. 라벨 텍스트 처리
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const bool has_label = (label != label_end);
    const ImVec2 label_size = has_label ? ImGui::CalcTextSize(label, label_end, true) : ImVec2(0.0f, 0.0f);

    // 2. 토글 스위치 크기 계산
    float height = ImGui::GetFrameHeight() * 0.8f;
    float width = height * 1.99f;

    float outer_radius = height * 0.50f;
    float inner_radius = outer_radius * 0.7f;

    // 3. 레이아웃 위치 및 영역 계산
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    float toggle_x_pos;
    ImVec2 total_size;

    if (has_label)
    {
        float avail_width = ImGui::GetContentRegionAvail().x;
        float min_width = label_size.x + style.ItemInnerSpacing.x + width;
        float actual_width = ImMax(avail_width, min_width);

        toggle_x_pos = cursor_pos.x + actual_width - width;
        total_size = ImVec2(actual_width, ImMax(label_size.y, height));
    }
    else
    {
        toggle_x_pos = cursor_pos.x;
        total_size = ImVec2(width, height);
    }

    // 4. 상호작용 버튼 배치 및 클릭 처리
    bool changed = false;

    ImRect total_bb(cursor_pos, ImVec2(cursor_pos.x + total_size.x, cursor_pos.y + total_size.y));

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    ImVec2 toggle_min = ImVec2(toggle_x_pos, cursor_pos.y + (total_size.y - height) * 0.5f);
    ImVec2 toggle_max = ImVec2(toggle_min.x + width, toggle_min.y + height);
    ImRect toggle_bb(toggle_min, toggle_max);

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(toggle_bb, id, &hovered, &held);

    if (pressed)
    {
        *v = !*v;
        changed = true;
    }

    // 5. StateStorage 기반 애니메이션 (Lerp)
    float anim_t = window->StateStorage.GetFloat(id, *v ? 1.0f : 0.0f);
    float target_t = *v ? 1.0f : 0.0f;

    float speed = 15.0f;
    anim_t = ImLerp(anim_t, target_t, ImGui::GetIO().DeltaTime * speed);

    if (ImAbs(anim_t - target_t) < 0.0001f)
        anim_t = target_t;

    window->StateStorage.SetFloat(id, anim_t);

    // 6. 렌더링
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (has_label)
    {
        float text_y = cursor_pos.y + (total_size.y - label_size.y) * 0.5f;
        draw_list->AddText(ImVec2(cursor_pos.x, text_y), ImGui::GetColorU32(ImGuiCol_Text), label, label_end);
    }

    ImVec2 p = ImVec2(toggle_x_pos, cursor_pos.y + (total_size.y - height) * 0.5f);

    // --- 자연스러운 색상 계산 부분 ---
    // OFF 상태 색상 (FrameBg 계열)
    const ImVec4 col_off_base   = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
    const ImVec4 col_off_hover  = ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered);
    const ImVec4 col_off_active = ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive);

    // ON 상태 색상 (Button 계열)
    const ImVec4 col_on_base    = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    const ImVec4 col_on_hover   = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    const ImVec4 col_on_active  = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

    // 토글 위치(anim_t)에 따른 단계별 색상 보간
    ImVec4 col_base   = ImLerp(col_off_base,   col_on_base,   anim_t);
    ImVec4 col_hover  = ImLerp(col_off_hover,  col_on_hover,  anim_t);
    ImVec4 col_active = ImLerp(col_off_active, col_on_active, anim_t);

    // 마우스 반응(Normal -> Hover -> Held) 상태 적용
    ImVec4 final_col = (held && hovered) ? col_active : (hovered ? col_hover : col_base);
    ImU32 col_bg = ImGui::GetColorU32(final_col);

    // 배경 및 테두리 그리기
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);

    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    float border_thickness = 1.5f;
    draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), col_border, height * 0.5f, 0, border_thickness);

    // 안쪽 원형 토글 그리기
    ImVec2 circle_center = ImVec2(p.x + outer_radius + anim_t * (width - outer_radius * 2.0f), p.y + outer_radius);
    draw_list->AddCircleFilled(circle_center, inner_radius, IM_COL32(255, 255, 255, 255));

    return changed;
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
    ImVec4 active_col_v     = ImGui::theme_id ? ImColor(87, 242, 135):ImColor(98, 192, 115);    // 부드러운 그린 (활성)
    ImVec4 inactive_col_v   = ImGui::theme_id ? ImColor(36, 36, 49) : ImColor(200, 200, 200);   // 다크 그레이 (비활성) [dark/white]
    ImVec4 hover_col_v      = ImGui::theme_id ? ImColor(44, 44, 47) : ImColor(180, 180, 180);   // 호버 색상


    ImU32 text_active_col   = ImGui::theme_id ? IM_COL32(240, 240, 240, 255) : IM_COL32(40, 40, 45, 255); // [dark/white]
    ImU32 text_inactive_col = ImGui::theme_id ? IM_COL32(130, 130, 130, 255) : IM_COL32(190, 190, 190, 255);
    ImU32 icon_col          = ImGui::theme_id ? IM_COL32(20, 20, 20, 255)    : IM_COL32(240, 240, 240, 240);

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


void ImGui::Help(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


