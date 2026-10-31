#pragma once
#include "imgui.h"
#include "raylib.h"
#include <functional>

namespace ImGui
{
    inline Camera camera = {
        { 1.0, 0.5, 2.0 },       // position
        { 0.0, 0.0, 0.0 },       // target
        { 0.0, 1.0, 0.0 },       // up
        45.0f,                   // fovy
        CAMERA_PERSPECTIVE       // projection
    };

    void DrawAxes(float length, float thickness);

    void Show3D(const char* title, bool open, std::function<void()> func);
    void Destroy3D();
}
