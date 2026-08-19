#include "ui_notify.h"
#include "ui_icon.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <chrono>
#include <cstdarg>
#include <cstdint>

// ==============================================================================
// 내부 설정 매크로
// ==============================================================================
#define NOTIFY_MAX_MSG_LENGTH           4096
#define NOTIFY_PADDING_Y                20.f        // 상단 Y 패딩
#define NOTIFY_FADE_IN_OUT_TIME         150
#define NOTIFY_DEFAULT_DISMISS          3000
#define NOTIFY_OPACITY                  1.0f
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

    static int g_next_toast_id = 0;

    struct ImGuiToastInternal
    {
        int id;
        ImGuiToastType type;
        char content[NOTIFY_MAX_MSG_LENGTH];
        int dismiss_time;
        uint64_t creation_time;

        ImGuiToastInternal(ImGuiToastType type, int dismiss_time, const char* format, va_list args)
            : id(g_next_toast_id++), type(type), dismiss_time(dismiss_time), creation_time(GetTickCount())
        {
            memset(this->content, 0, sizeof(this->content));
            vsnprintf(this->content, sizeof(this->content), format, args);
        }

        const ImVec4 GetColor() const
        {
            switch (this->type)
            {
                case ImGuiToastType_Success: return { 98, 192, 115, 255 };
                case ImGuiToastType_Warning: return { 227, 179, 65, 255 };
                case ImGuiToastType_Error:   return { 224, 108, 117, 255 };
                case ImGuiToastType_Info:    return { 100, 141, 222, 255 };
                default:                     return { 220, 224, 230, 255 };
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
                case ImGuiToastType_Success: return { 16, 28, 18, 255 };
                case ImGuiToastType_Warning: return { 28, 25, 16, 255 };
                case ImGuiToastType_Error:   return { 31, 16, 18, 255 };
                case ImGuiToastType_Info:    return { 16, 20, 31, 255 };
                default:                     return { 20, 21, 23, 255 };
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

    static std::vector<ImGuiToastInternal> g_notifications;

    // 공통 알림 추가 함수
    void AddNotification(ImGuiToastType type, const char* format, va_list args)
    {
        // [수정] 스택처럼 쌓이지 않도록, 새 알람이 오면 기존 알람을 덮어씌웁니다.
        g_notifications.clear();
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
    // 렌더링 로직
    // ==============================================================================
    void RenderNotifications()
    {
        const auto viewport = ImGui::GetMainViewport();
        const auto vp_pos = viewport->WorkPos;
        const auto vp_size = viewport->WorkSize;

        ImGuiContext& g = *GImGui;

        int remove_index = -1;

        // 알림은 최대 1개만 존재하지만 구조를 유지하기 위해 루프 사용
        for (int i = 0; i < (int)g_notifications.size(); i++)
        {
            auto* current_toast = &g_notifications[i];

            if (current_toast->GetPhase() == ImGuiToastPhase_Expired)
            {
                remove_index = i;
                continue;
            }

            const auto icon = current_toast->GetIcon();
            const auto content = current_toast->content;
            const auto phase = current_toast->GetPhase();
            const auto opacity = current_toast->GetFadePercent();

            // [수정] 상단 중앙 애니메이션: 위에서 아래로 살짝 내려옴
            float nudge_y = 0.0f;
            if (phase == ImGuiToastPhase_FadeIn || phase == ImGuiToastPhase_FadeOut)
            {
                nudge_y = -15.0f * (1.0f - opacity); // 투명할수록 위쪽(-방향)으로 당겨짐
            }

            // [수정] 상단 중앙 X, Y 계산
            float draw_x = vp_pos.x + (vp_size.x * 0.5f); // 중앙
            float draw_y = vp_pos.y + NOTIFY_PADDING_Y + nudge_y; // 상단 + 패딩 + 애니메이션

            auto text_color = current_toast->GetColor();
            if (text_color.x > 1.0f || text_color.y > 1.0f || text_color.z > 1.0f) {
                text_color.x /= 255.0f; text_color.y /= 255.0f; text_color.z /= 255.0f;
            }
            text_color.w = opacity;

            auto raw_bg_color = current_toast->GetBgColor();
            ImVec4 bg_color = ImVec4(raw_bg_color.x / 255.0f, raw_bg_color.y / 255.0f, raw_bg_color.z / 255.0f, opacity);

            PushStyleColor(ImGuiCol_WindowBg, bg_color);
            PushStyleColor(ImGuiCol_Border, text_color);
            PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));

            SetNextWindowBgAlpha(opacity);

            char window_name[50]{};
            snprintf(window_name, sizeof(window_name), "##TOAST%d", current_toast->id);

            // [수정] Pivot을 상단 중앙(0.5, 0.0)으로 설정
            SetNextWindowPos(ImVec2(draw_x, draw_y), ImGuiCond_Always, ImVec2(0.5f, -1.0f));

            ImGui::SetNextWindowViewport(viewport->ID);

            Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

            {
                PushTextWrapPos(vp_size.x / 2.f); // 중앙 배치를 고려해 텍스트 랩핑 영역 확장
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

            float progress = current_toast->GetProgressPercent();
            if (progress > 0.0f)
            {
                ImVec2 win_pos = GetWindowPos();
                ImVec2 win_size = GetWindowSize();
                float bar_height = 3.0f;

                ImVec2 p_min = ImVec2(win_pos.x + 1.4f, win_pos.y + win_size.y - bar_height);
                ImVec2 p_max = ImVec2(win_pos.x + (win_size.x * progress), win_pos.y + win_size.y);

                GetWindowDrawList()->AddRectFilled(p_min, p_max, ColorConvertFloat4ToU32(text_color), 4.0f, ImDrawFlags_RoundCornersBottom);
            }

            End();

            PopStyleVar(3);
            PopStyleColor(2);
        }

        if (remove_index != -1)
        {
            g_notifications.erase(g_notifications.begin() + remove_index);
        }
    }
}