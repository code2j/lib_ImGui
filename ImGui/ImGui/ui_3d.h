#pragma once
#include "imgui.h"
#include "raylib.h"
#include <functional>

// Commands are submitted each frame from the Context UI callback.
namespace ImGui3D
{
    void Cube(Vector3 position = {0, 0, 0}, Vector3 size = {1, 1, 1}, Color color = WHITE);
    void Sphere(Vector3 center = {0, 0, 0}, float radius = 0.5f, Color color = WHITE);
    void Line(Vector3 start, Vector3 end, Color color = WHITE);
    void ClearFrame();
    void RenderFrame();
    inline Camera camera = {
        { 1.0, 0.5, 2.0 },       // position
        { 0.0, 0.0, 0.0 },       // target
        { 0.0, 1.0, 0.0 },       // up
        45.0f,                   // fovy
        CAMERA_PERSPECTIVE       // projection
    };

    void DrawAxes(float length, float thickness);

    void Show3D(const char* title, bool open, std::function<void()> func);
    bool Get3DClick(Vector2& position);
    bool Get3DMouse(Vector2& position);
    void Destroy3D();
}
