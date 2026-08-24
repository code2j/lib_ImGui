#pragma once
#include "imgui.h"

namespace ImGui
{
    void NotifySucc(const char* format, ...);
    void NotifyInfo(const char* format, ...);
    void NotifyWarn(const char* format, ...);
    void NotifyError(const char* format, ...);

    void RenderNotifications();
}