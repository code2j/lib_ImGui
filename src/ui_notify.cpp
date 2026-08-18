#include "ui_notify.h"
#include "ui_icon.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <chrono>
#include <cstdarg>

// ==============================================================================
// 내부 설정 매크로 (이제 cpp 내부에서만 사용됨)
// ==============================================================================
#define NOTIFY_MAX_MSG_LENGTH           4096        // 최대 메시지 길이
#define NOTIFY_PADDING_X                20.f        // 우하단 X 패딩
#define NOTIFY_PADDING_Y                20.f        // 우하단 Y 패딩
#define NOTIFY_PADDING_MESSAGE_Y        10.f        // 메시지 간 Y 패딩
#define NOTIFY_FADE_IN_OUT_TIME         150         // 페이드 인/아웃 시간 (ms)
#define NOTIFY_DEFAULT_DISMISS          3000        // 기본 유지 시간 (ms)
#define NOTIFY_OPACITY                  1.0f        // 기본 투명도
#define NOTIFY_TOAST_FLAGS              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_Tooltip

namespace
{
    // ==============================================================================
    // 내부 데이터 구조체 및 열거형
    // ==============================================================================
    enum ImGuiToastType
    {
        ImGuiToastType_Success,
        ImGuiToastType_Warning,
        ImGuiToastType_Error,
        ImGuiToastType_Info
    };

    enum ImGuiToastPhase
    {
        ImGuiToastPhase_FadeIn,
        ImGuiToastPhase_Wait,
        ImGuiToastPhase_FadeOut,
        ImGuiToastPhase_Expired
    };

    static uint64_t GetTickCount()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    struct ImGuiToastInternal
    {
        ImGuiToastType type;
        char content[NOTIFY_MAX_MSG_LENGTH];
        int dismiss_time;
        uint64_t creation_time;

        ImGuiToastInternal(ImGuiToastType type, int dismiss_time, const char* format, va_list args)
            : type(type), dismiss_time(dismiss_time), creation_time(GetTickCount())
        {
            memset(this->content, 0, sizeof(this->content));
            vsnprintf(this->content, sizeof(this->content), format, args);
        }

        const ImVec4 GetColor() const
        {
            switch (this->type)
            {
                case ImGuiToastType_Success: return { 98, 192, 115, 255 };  // Soft Green
                case ImGuiToastType_Warning: return { 227, 179, 65, 255 };  // Soft Yellow/Amber
                case ImGuiToastType_Error:   return { 224, 108, 117, 255 }; // Soft Red/Coral
                case ImGuiToastType_Info:    return { 100, 141, 222, 255 }; // Blue
                default:                     return { 220, 224, 230, 255 }; // Soft White/Gray
            }
        }

        const char* GetIcon() const
        {
            switch (this->type)
            {
                case ImGuiToastType_Success: return ICON_MD_CHECK_CIRCLE;
                case ImGuiToastType_Warning: return ICON_MD_WARNING;
                case ImGuiToastType_Error:   return ICON_MD_ERROR;
                case ImGuiToastType_Info:    return ICON_MD_INFO;
                default:                     return NULL;
            }
        }

        const ImVec4 GetBgColor() const
        {
            switch (this->type)
            {
                case ImGuiToastType_Success: return { 16, 28, 18, 255 }; // 은은한 다크 그린
                case ImGuiToastType_Warning: return { 28, 25, 16, 255 }; // 은은한 다크 옐로우
                case ImGuiToastType_Error:   return { 31, 16, 18, 255 }; // 은은한 다크 레드
                case ImGuiToastType_Info:    return { 16, 20, 31, 255 }; // 다크 블루
                default:                     return { 20, 21, 23, 255 }; // 기본 다크 그레이
            }
        }

        uint64_t GetElapsedTime() const { return GetTickCount() - this->creation_time; }

        ImGuiToastPhase GetPhase() const
        {
            const auto elapsed = GetElapsedTime();
            if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time + NOTIFY_FADE_IN_OUT_TIME)
                return ImGuiToastPhase_Expired;
            else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time)
                return ImGuiToastPhase_FadeOut;
            else if (elapsed > NOTIFY_FADE_IN_OUT_TIME)
                return ImGuiToastPhase_Wait;
            else
                return ImGuiToastPhase_FadeIn;
        }

        float GetFadePercent() const
        {
            const auto phase = GetPhase();
            const auto elapsed = GetElapsedTime();

            if (phase == ImGuiToastPhase_FadeIn)
                return ((float)elapsed / (float)NOTIFY_FADE_IN_OUT_TIME) * NOTIFY_OPACITY;
            else if (phase == ImGuiToastPhase_FadeOut)
                return (1.f - (((float)elapsed - (float)NOTIFY_FADE_IN_OUT_TIME - (float)this->dismiss_time) / (float)NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;

            return 1.f * NOTIFY_OPACITY;
        }

        float GetProgressPercent() const
        {
            const auto elapsed = GetElapsedTime();
            const auto total_time = NOTIFY_FADE_IN_OUT_TIME * 2 + this->dismiss_time;

            if (elapsed >= total_time)
                return 0.0f;

            return 1.0f - ((float)elapsed / (float)total_time);
        }
    };

    // 알림 목록을 관리하는 내부 전역 변수
    static std::vector<ImGuiToastInternal> g_notifications;

    // 공통 알림 추가 함수
    void AddNotification(ImGuiToastType type, const char* format, va_list args)
    {
        g_notifications.emplace_back(type, NOTIFY_DEFAULT_DISMISS, format, args);
    }
}

namespace ImGui
{
    // ==============================================================================
    // 외부 노출 인터페이스 구현
    // ==============================================================================

    void NotifySucc(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AddNotification(ImGuiToastType_Success, format, args);
        va_end(args);
    }

    void NotifyWarn(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AddNotification(ImGuiToastType_Warning, format, args);
        va_end(args);
    }

    void NotifyError(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AddNotification(ImGuiToastType_Error, format, args);
        va_end(args);
    }

    void NotifyInfo(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AddNotification(ImGuiToastType_Info, format, args);
        va_end(args);
    }

    // ==============================================================================
    // 렌더링 로직 (기존 애니메이션 및 UI 코드 유지)
    // ==============================================================================
    void RenderNotifications()
    {
        const auto viewport = GetMainViewport();
        const auto vp_pos = viewport->WorkPos;
        const auto vp_size = viewport->WorkSize;

        ImGuiContext& g = *GImGui;
        float delta_time = ImGui::GetIO().DeltaTime;
        float anim_speed = 12.0f;

        float target_height = 0.f;
        int remove_index = -1;

        for (auto i = 0; i < g_notifications.size(); i++)
        {
            auto* current_toast = &g_notifications[i];

            if (current_toast->GetPhase() == ImGuiToastPhase_Expired)
            {
                if (remove_index == -1) remove_index = i;
                continue;
            }

            const auto icon = current_toast->GetIcon();
            const auto content = current_toast->content;
            const auto opacity = current_toast->GetFadePercent();

            auto text_color = current_toast->GetColor();
            if (text_color.x > 1.0f || text_color.y > 1.0f || text_color.z > 1.0f) {
                text_color.x /= 255.0f;
                text_color.y /= 255.0f;
                text_color.z /= 255.0f;
            }
            text_color.w = opacity;

            char window_name[50]{};
            snprintf(window_name, sizeof(window_name), "##TOAST%d", i);

            // 위치 보간
            ImGuiID toast_id = ImGui::GetID((const void*)current_toast);
            ImGuiStorage* storage = ImGui::GetStateStorage();
            float current_height = storage->GetFloat(toast_id, target_height);

            current_height = ImLerp(current_height, target_height, delta_time * anim_speed);
            if (ImAbs(current_height - target_height) < 0.1f)
                current_height = target_height;

            storage->SetFloat(toast_id, current_height);

            auto raw_bg_color = current_toast->GetBgColor();
            ImVec4 bg_color = ImVec4(raw_bg_color.x / 255.0f, raw_bg_color.y / 255.0f, raw_bg_color.z / 255.0f, opacity);
            ImVec4 border_color = text_color;

            PushStyleColor(ImGuiCol_WindowBg, bg_color);
            PushStyleColor(ImGuiCol_Border, border_color);
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

            SetNextWindowBgAlpha(opacity);
            SetNextWindowPos(ImVec2(vp_pos.x + vp_size.x - NOTIFY_PADDING_X, vp_pos.y + vp_size.y - NOTIFY_PADDING_Y - current_height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

            Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

            {
                PushTextWrapPos(vp_size.x / 3.f);
                bool was_title_rendered = false;

                if (icon && strlen(icon) > 0)
                {
                    TextColored(text_color, "%s", icon);
                    was_title_rendered = true;
                }

                if (content && strlen(content) > 0)
                {
                    if (was_title_rendered) SameLine(0.0f, 10.0f);
                    Text("%s", content);
                }

                PopTextWrapPos();
            }

            // 프로그레스 바 렌더링
            float progress = current_toast->GetProgressPercent();
            if (progress > 0.0f)
            {
                ImVec2 win_pos = GetWindowPos();
                ImVec2 win_size = GetWindowSize();
                float bar_height = 3.0f;

                ImVec2 p_min = ImVec2(win_pos.x + 1.4f, win_pos.y + win_size.y - bar_height);
                ImVec2 p_max = ImVec2(win_pos.x + (win_size.x * progress), win_pos.y + win_size.y);

                ImU32 bar_color = ColorConvertFloat4ToU32(text_color);
                GetWindowDrawList()->AddRectFilled(p_min, p_max, bar_color, 4.0f, ImDrawFlags_RoundCornersBottom);
            }

            target_height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

            End();

            PopStyleVar(3);
            PopStyleColor(2);
        }

        // 만료된 토스트 제거
        if (remove_index != -1)
        {
            g_notifications.erase(g_notifications.begin() + remove_index);
        }
    }
}