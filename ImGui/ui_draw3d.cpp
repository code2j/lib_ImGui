#include "ui_draw3d.h"
#include "raylib.h"

namespace ImGui
{
    void draw_axes(float length, float thickness)
    {
        const int sides         = 8;         // 원기둥의 면 개수
        const Vector3 origin    = { 0.0f, 0.0f, 0.0f };

        // X축 (빨강)
        DrawCylinderEx(origin, (Vector3){ length, 0.0f, 0.0f }, thickness, thickness, sides, RED);

        // Y축 (녹색)
        DrawCylinderEx(origin, (Vector3){ 0.0f, length, 0.0f }, thickness, thickness, sides, GREEN);

        // Z축 (파랑)
        DrawCylinderEx(origin, (Vector3){ 0.0f, 0.0f, length }, thickness, thickness, sides, BLUE);
    }
}
