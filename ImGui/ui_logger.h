#pragma once

namespace ImGui
{
    void InitLogger();
    void ShutdownLogger();
    void ClearLogger();
    void ShowLogger(const char* title);
}
