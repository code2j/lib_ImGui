#include "ui_notify.h"

#include "imgui_internal.h"

void ImGuiToast::set_title(const char *format, va_list args) { vsnprintf(this->title, sizeof(this->title), format, args); }

void ImGuiToast::set_content(const char *format, va_list args) { vsnprintf(this->content, sizeof(this->content), format, args); }

auto ImGuiToast::get_title() -> char * { return this->title; }






auto ImGuiToast::get_color() -> const ImVec4
{
    switch (this->type)
    {
        case ImGuiToastType_None:
            return { 220, 224, 230, 255 }; // Soft White/Gray
        case ImGuiToastType_Success:
            return { 98, 192, 115, 255 };  // Soft Green
        case ImGuiToastType_Warning:
            return { 227, 179, 65, 255 };  // Soft Yellow/Amber
        case ImGuiToastType_Error:
            return { 224, 108, 117, 255 }; // Soft Red/Coral
        case ImGuiToastType_Info:
            return { 100, 141, 222, 255 }; // Blue (기준 색상)
        default:
            return { 220, 224, 230, 255 }; // Soft White/Gray
    }
}

auto ImGuiToast::get_icon() -> const char *
{
    switch (this->type)
    {
        case ImGuiToastType_None:
            return NULL;
        case ImGuiToastType_Success:
            return ICON_MD_CHECK_CIRCLE;
        case ImGuiToastType_Warning:
            return ICON_MD_WARNING;
        case ImGuiToastType_Error:
            return ICON_MD_ERROR;
        case ImGuiToastType_Info:
            return ICON_MD_INFO;
        default:
            return NULL;
    }
}

auto ImGuiToast::get_bg_color() -> const ImVec4
{
    switch (this->type)
    {
        case ImGuiToastType_None:
            return { 20, 21, 23, 255 }; // 은은한 다크 그레이
        case ImGuiToastType_Success:
            return { 16, 28, 18, 255 }; // 은은한 다크 그린
        case ImGuiToastType_Warning:
            return { 28, 25, 16, 255 }; // 은은한 다크 옐로우
        case ImGuiToastType_Error:
            return { 31, 16, 18, 255 }; // 은은한 다크 레드
        case ImGuiToastType_Info:
            return { 16, 20, 31, 255 }; // 요청하신 다크 블루
        default:
            return { 20, 21, 23, 255 }; // 기본 다크 그레이
    }
}

auto ImGuiToast::get_content() -> char * { return this->content; }

auto ImGuiToast::get_elapsed_time() { return get_tick_count() - this->creation_time; }

auto ImGuiToast::get_phase() -> const ImGuiToastPhase
{
    const auto elapsed = get_elapsed_time();

    if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time + NOTIFY_FADE_IN_OUT_TIME)
    {
        return ImGuiToastPhase_Expired;
    }
    else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time)
    {
        return ImGuiToastPhase_FadeOut;
    }
    else if (elapsed > NOTIFY_FADE_IN_OUT_TIME)
    {
        return ImGuiToastPhase_Wait;
    }
    else
    {
        return ImGuiToastPhase_FadeIn;
    }
}

auto ImGuiToast::get_fade_percent() -> const float
{
    const auto phase = get_phase();
    const auto elapsed = get_elapsed_time();

    if (phase == ImGuiToastPhase_FadeIn)
    {
        return ((float)elapsed / (float)NOTIFY_FADE_IN_OUT_TIME) * NOTIFY_OPACITY;
    }
    else if (phase == ImGuiToastPhase_FadeOut)
    {
        return (1.f - (((float)elapsed - (float)NOTIFY_FADE_IN_OUT_TIME - (float)this->dismiss_time) / (float)NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;
    }

    return 1.f * NOTIFY_OPACITY;
}

auto ImGuiToast::get_tick_count() -> const unsigned long long
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

ImGuiToast::ImGuiToast(ImGuiToastType type, int dismiss_time)
{
    IM_ASSERT(type < ImGuiToastType_COUNT);

    this->type = type;
    this->dismiss_time = dismiss_time;
    this->creation_time = get_tick_count();

    memset(this->title, 0, sizeof(this->title));
    memset(this->content, 0, sizeof(this->content));
}

auto ImGuiToast::get_progress_percent() -> const float
{
    const auto elapsed = get_elapsed_time();
    const auto total_time = NOTIFY_FADE_IN_OUT_TIME * 2 + this->dismiss_time;

    if (elapsed >= total_time)
        return 0.0f;

    return 1.0f - ((float)elapsed / (float)total_time);
}

namespace ImGui
{
    void InsertNotification(const ImGuiToast &toast)
    {
        notifications.push_back(toast);
    }

    void RemoveNotification(int index)
    {
        notifications.erase(notifications.begin() + index);
    }



    void RenderNotifications()
    {
        const auto viewport = GetMainViewport();
        const auto vp_pos = viewport->WorkPos;
        const auto vp_size = viewport->WorkSize;

        ImGuiContext& g = *GImGui;
        float delta_time = ImGui::GetIO().DeltaTime;
        float anim_speed = 12.0f; // 위치 이동 애니메이션 속도 (클수록 빠름)

        float target_height = 0.f;

        // 만료된 토스트를 안전하게 삭제하기 위한 지연 삭제 인덱스
        int remove_index = -1;

        for (auto i = 0; i < notifications.size(); i++)
        {
            auto* current_toast = &notifications[i];

            // 만료 체크
            if (current_toast->get_phase() == ImGuiToastPhase_Expired)
            {
                if (remove_index == -1) remove_index = i;
                continue;
            }

            const auto icon = current_toast->get_icon();
            const auto content = current_toast->get_content();
            const auto opacity = current_toast->get_fade_percent();

            auto text_color = current_toast->get_color();
            if (text_color.x > 1.0f || text_color.y > 1.0f || text_color.z > 1.0f) {
                text_color.x /= 255.0f;
                text_color.y /= 255.0f;
                text_color.z /= 255.0f;
            }
            text_color.w = opacity;

            char window_name[50]{};
            snprintf(window_name, sizeof(window_name), "##TOAST%d", i);

            // =======================================================
            // 위치 보간 (Smooth Y-Position Animation)
            // =======================================================
            // 알림 고유 ID 생성을 위해 토스트 포인터를 ID로 사용
            ImGuiID toast_id = ImGui::GetID((const void*)current_toast);
            ImGuiStorage* storage = ImGui::GetStateStorage();

            // 현재 애니메이션 적용 중인 height 위치 값 가져오기 (처음 생성 시 target_height로 초기화)
            float current_height = storage->GetFloat(toast_id, target_height);

            // 부드러운 위치 보간 (Target Y 위치로 슬라이딩)
            current_height = ImLerp(current_height, target_height, delta_time * anim_speed);
            if (ImAbs(current_height - target_height) < 0.1f)
                current_height = target_height;

            storage->SetFloat(toast_id, current_height);
            // =======================================================

            auto raw_bg_color = current_toast->get_bg_color();
            ImVec4 bg_color = ImVec4(
                raw_bg_color.x / 255.0f,
                raw_bg_color.y / 255.0f,
                raw_bg_color.z / 255.0f,
                opacity
            );

            ImVec4 border_color = text_color;

            PushStyleColor(ImGuiCol_WindowBg, bg_color);
            PushStyleColor(ImGuiCol_Border, border_color);
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

            SetNextWindowBgAlpha(opacity);
            // 고정 height 대신 보간된 current_height를 적용
            SetNextWindowPos(ImVec2(vp_pos.x + vp_size.x - NOTIFY_PADDING_X, vp_pos.y + vp_size.y - NOTIFY_PADDING_Y - current_height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

            Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

            {
                PushTextWrapPos(vp_size.x / 3.f);

                bool was_title_rendered = false;

                if (!NOTIFY_NULL_OR_EMPTY(icon))
                {
                    TextColored(text_color, "%s", icon);
                    was_title_rendered = true;
                }

                if (!NOTIFY_NULL_OR_EMPTY(content))
                {
                    if (was_title_rendered)
                    {
                        SameLine(0.0f, 10.0f);
                    }
                    Text("%s", content);
                }

                PopTextWrapPos();
            }

            // =======================================================
            // 프로그레스 바 렌더링 (오른쪽에서 왼쪽으로 줄어듦)
            // =======================================================
            float progress = current_toast->get_progress_percent();
            if (progress > 0.0f)
            {
                ImVec2 win_pos = GetWindowPos();
                ImVec2 win_size = GetWindowSize();
                float bar_height = 3.0f; // 프로그레스 바 두께 (원하는 수치로 조절 가능)

                // 오른쪽 끝이 왼쪽으로 줄어드는 형태 (왼쪽은 고정, 오른쪽 끝이 진행률에 따라 감소)
                ImVec2 p_min = ImVec2(win_pos.x+1.4, win_pos.y + win_size.y - bar_height);
                ImVec2 p_max = ImVec2(win_pos.x + (win_size.x * progress), win_pos.y + win_size.y);

                ImU32 bar_color = ColorConvertFloat4ToU32(text_color);
                GetWindowDrawList()->AddRectFilled(p_min, p_max, bar_color, 4.0f, ImDrawFlags_RoundCornersBottom);
            }
            // =======================================================

            // 다음 토스트가 올라갈 목표 Y 위치 누적
            target_height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

            End();

            PopStyleVar(3);
            PopStyleColor(2);
        }

        // 만료된 토스트 제거
        if (remove_index != -1)
        {
            RemoveNotification(remove_index);
        }
    }
}

