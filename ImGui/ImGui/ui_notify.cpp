#include "ui_notify.h"
#include "ui_font_icon.h"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <algorithm>
#include <cmath>

// ==============================================================================
// 내부 설정 매크로
// ==============================================================================
#define NOTIFY_MAX_MSG_LENGTH           4096
#define NOTIFY_PADDING_Y                12.5f        // 상단 Y 패딩
#define NOTIFY_FADE_IN_OUT_TIME         300
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
    void ShowNotifications()
    {
        if (g_notifications.empty()) return;
        auto& toast = g_notifications.back();
        if (toast.GetPhase() == ImGuiToastPhase_Expired) {
            g_notifications.clear();
            return;
        }
        const ImGuiWindow* menu = FindWindowByName("window");
        ImGuiViewport* viewport = menu && menu->Viewport ? menu->Viewport : GetMainViewport();
        const ImVec2 origin = menu ? menu->Pos : viewport->WorkPos;
        const ImVec2 area = menu ? menu->Size : viewport->WorkSize;
        const float width = std::max(1.0f, std::min(440.0f, area.x - 24.0f));
        const float elapsed = static_cast<float>(toast.GetElapsedTime());
        const float progress = std::max(0.0f, std::min(1.0f,
            (elapsed - NOTIFY_FADE_IN_OUT_TIME) / toast.dismiss_time));
        const float alpha = std::max(0.0f, std::min(1.0f, toast.GetFadePercent()));
        const bool exiting = toast.GetPhase() == ImGuiToastPhase_FadeOut;
        const float glitch = exiting ? (1.0f - alpha) * 7.0f * std::sin(elapsed * 0.11f) : 0.0f;
        const ImVec2 pos(origin.x + (area.x - width) * 0.5f + glitch,
                         origin.y + NOTIFY_PADDING_Y - (1.0f - alpha) * 10.0f);
        const float height = 72.0f;
        SetNextWindowPos(pos);
        SetNextWindowSize(ImVec2(width, height));
        SetNextWindowViewport(viewport->ID);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        SetNextWindowBgAlpha(0.0f);
        Begin("##notification_hud", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
              ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_Tooltip);
        ImDrawList* draw = GetWindowDrawList();
        const auto color = [alpha](int r, int g, int b, int a = 255) {
            return IM_COL32(r, g, b, static_cast<int>(a * alpha));
        };
        // 잘린 모서리, 경고색 포인트와 스캔 라인으로 미래형 HUD를 구성한다.
        const ImVec2 points[] = { ImVec2(pos.x + 12, pos.y), ImVec2(pos.x + width, pos.y),
            ImVec2(pos.x + width, pos.y + height - 10), ImVec2(pos.x + width - 10, pos.y + height),
            ImVec2(pos.x, pos.y + height), ImVec2(pos.x, pos.y + 12) };
        draw->AddConvexPolyFilled(points, 6, color(17, 24, 28, 240));
        draw->AddPolyline(points, 6, color(246, 224, 70, 210), ImDrawFlags_Closed, 1.0f);
        draw->AddRectFilled(ImVec2(pos.x, pos.y + 13), ImVec2(pos.x + 3, pos.y + height - 8), color(246, 224, 70));
        const char* category = toast.type == ImGuiToastType_Error ? "SYSTEM // ERROR" :
            toast.type == ImGuiToastType_Warning ? "SYSTEM // WARNING" :
            toast.type == ImGuiToastType_Success ? "SYSTEM // COMPLETE" : "SYSTEM // NOTIFICATION";
        draw->AddText(ImVec2(pos.x + 15, pos.y + 8), color(246, 224, 70), category);
        draw->PushClipRect(ImVec2(pos.x + 15, pos.y + 28), ImVec2(pos.x + width - 12, pos.y + 51), true);
        draw->AddText(ImVec2(pos.x + 15, pos.y + 29), color(235, 242, 234), toast.content);
        draw->PopClipRect();
        const float left = pos.x + 15, right = pos.x + width - 15, bar_y = pos.y + height - 11;
        draw->AddRectFilled(ImVec2(left, bar_y), ImVec2(right, bar_y + 3), color(69, 72, 50));
        if (progress > 0.0f)
            draw->AddRectFilled(ImVec2(left, bar_y), ImVec2(left + (right - left) * progress, bar_y + 3), color(246, 224, 70));
        if (exiting) {
            const float sweep = (1.0f - alpha) * width;
            draw->AddRectFilled(ImVec2(pos.x, pos.y + 23), ImVec2(pos.x + sweep, pos.y + 25), color(94, 230, 225, 180));
        }
        End();
        PopStyleVar(2);
    }
}
