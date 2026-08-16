#include "ui_notify.h"

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

        float height = 0.f;

        for (auto i = 0; i < notifications.size(); i++)
        {
            auto* current_toast = &notifications[i];

            // 만료된 토스트 제거
            if (current_toast->get_phase() == ImGuiToastPhase_Expired)
            {
                RemoveNotification(i);
                continue;
            }

            const auto icon = current_toast->get_icon();
            const auto content = current_toast->get_content();
            const auto opacity = current_toast->get_fade_percent();

            // 색상 가져오기 및 0.0f ~ 1.0f 범위로 정규화 (기존 코드가 0~255를 반환하므로 보정)
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
            // 이미지 스타일 커스텀 적용 시작
            // =======================================================
            auto raw_bg_color = current_toast->get_bg_color();
            ImVec4 bg_color = ImVec4(
                raw_bg_color.x / 255.0f,
                raw_bg_color.y / 255.0f,
                raw_bg_color.z / 255.0f,
                opacity
            );

            ImVec4 border_color = text_color; // 테두리는 기존 텍스트/아이콘 색상과 동일하게 유지

            PushStyleColor(ImGuiCol_WindowBg, bg_color);
            PushStyleColor(ImGuiCol_Border, border_color);
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); // 테두리 두께 1px
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);   // 둥근 모서리
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f)); // 상하좌우 넉넉한 내부 여백
            // =======================================================

            SetNextWindowBgAlpha(opacity);
            SetNextWindowPos(ImVec2(vp_pos.x + vp_size.x - NOTIFY_PADDING_X, vp_pos.y + vp_size.y - NOTIFY_PADDING_Y - height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

            Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

            {
                PushTextWrapPos(vp_size.x / 3.f);

                bool was_title_rendered = false;

                // 아이콘 렌더링
                if (!NOTIFY_NULL_OR_EMPTY(icon))
                {
                    TextColored(text_color, "%s", icon);
                    was_title_rendered = true;
                }


                // 컨텐츠 렌더링 (단일 줄 구성으로 자연스럽게 이어지도록 처리)
                if (!NOTIFY_NULL_OR_EMPTY(content))
                {
                    if (was_title_rendered)
                    {
                        // 타이틀/아이콘 바로 옆에 텍스트가 오도록 간격 조정
                        SameLine(0.0f, 10.0f);
                    }
                    Text("%s", content);
                }

                PopTextWrapPos();
            }

            height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

            End();

            // =======================================================
            // 스타일 덮어쓰기 해제 (다른 UI에 영향을 주지 않도록 복구)
            // =======================================================
            PopStyleVar(3);
            PopStyleColor(2);
        }
    }
}

