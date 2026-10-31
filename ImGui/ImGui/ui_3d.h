#pragma once
#include "imgui.h"
#include <functional>

namespace ImGui
{
    void DrawAxes(float length, float thickness);

    void Show3D(const char* title, bool open, std::function<void()> func);
    void Destroy3D();
}
