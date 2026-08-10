// imgui-notify by patrickcjk
// https://github.com/patrickcjk/imgui-notify

#ifndef IMGUI_NOTIFY
#define IMGUI_NOTIFY

#pragma once
#include <vector>
#include <string>
#include <chrono>
#include "imgui.h"
#include "ui_icon.h"

#define NOTIFY_MAX_MSG_LENGTH			4096		// Max message content length
#define NOTIFY_PADDING_X				20.f		// Bottom-left X padding
#define NOTIFY_PADDING_Y				20.f		// Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y		10.f		// Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME			150			// Fade in and out duration
#define NOTIFY_DEFAULT_DISMISS			3000		// Auto dismiss after X ms (default, applied only of no data provided in constructors)
#define NOTIFY_OPACITY					1.0f		// 0-1 Toast opacity
#define NOTIFY_TOAST_FLAGS           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_Tooltip// Comment out if you don't want any separator between title and content
#define NOTIFY_USE_SEPARATOR

#define NOTIFY_INLINE					inline
#define NOTIFY_NULL_OR_EMPTY(str)		(!str ||! strlen(str))
#define NOTIFY_FORMAT(fn, format, ...)	if (format) { va_list args; va_start(args, format); fn(format, args, ##__VA_ARGS__); va_end(args); }

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;
typedef int ImGuiToastPos;

enum ImGuiToastType_
{
	ImGuiToastType_None,
	ImGuiToastType_Success,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Info,
	ImGuiToastType_COUNT
};

enum ImGuiToastPhase_
{
	ImGuiToastPhase_FadeIn,
	ImGuiToastPhase_Wait,
	ImGuiToastPhase_FadeOut,
	ImGuiToastPhase_Expired,
	ImGuiToastPhase_COUNT
};

enum ImGuiToastPos_
{
	ImGuiToastPos_TopLeft,
	ImGuiToastPos_TopCenter,
	ImGuiToastPos_TopRight,
	ImGuiToastPos_BottomLeft,
	ImGuiToastPos_BottomCenter,
	ImGuiToastPos_BottomRight,
	ImGuiToastPos_Center,
	ImGuiToastPos_COUNT
};

class ImGuiToast
{
private:
	ImGuiToastType	type = ImGuiToastType_None;
	char			title[NOTIFY_MAX_MSG_LENGTH];
	char			content[NOTIFY_MAX_MSG_LENGTH];
	int				dismiss_time = NOTIFY_DEFAULT_DISMISS;
	uint64_t		creation_time = 0;

private:
	// Setters
    void set_title(const char* format, va_list args);
    void set_content(const char* format, va_list args);

public:
	// Getters
    auto get_title() -> char* ;
    auto get_default_title() -> const char*;
    auto get_color() -> const ImVec4;
    auto get_icon() -> const char*;
    auto get_content() -> char* ;
    auto get_elapsed_time();
    auto get_phase() -> const ImGuiToastPhase;
    auto get_fade_percent() -> const float;
    static auto get_tick_count() -> const unsigned long long;

public:
	// Constructors
	ImGuiToast(ImGuiToastType type, int dismiss_time = NOTIFY_DEFAULT_DISMISS);
    ImGuiToast(ImGuiToastType type, const char* format, ...) : ImGuiToast(type) { NOTIFY_FORMAT(this->set_content, format); }
	ImGuiToast(ImGuiToastType type, int dismiss_time, const char* format, ...) : ImGuiToast(type, dismiss_time) { NOTIFY_FORMAT(this->set_content, format); }
};


namespace ImGui
{
	NOTIFY_INLINE std::vector<ImGuiToast> notifications;

    void InsertNotification(const ImGuiToast& toast);
    void RemoveNotification(int index);
    void RenderNotifications();
}

#endif