#pragma once
#include "raylib.h"

inline void DrawWorldAxesThick(float length, float thickness)
{
    Vector3 origin = { 0.0f, 0.0f, 0.0f };

    // 원기둥의 면 개수
    int sides = 8;

    // X축 (빨강)
    DrawCylinderEx(origin, (Vector3){ length, 0.0f, 0.0f }, thickness, thickness, sides, RED);

    // Y축 (녹색)
    DrawCylinderEx(origin, (Vector3){ 0.0f, length, 0.0f }, thickness, thickness, sides, GREEN);

    // Z축 (파랑)
    DrawCylinderEx(origin, (Vector3){ 0.0f, 0.0f, length }, thickness, thickness, sides, BLUE);
}