#include "ui_3d.h"
#include "raylib.h"
#include "imgui.h"
#include "ui.hpp"
#include "ui_font_icon.h"

namespace {
    const float WIDTH_3D= 1280;
    const float HEIGHT_3D= 720;

    RenderTexture2D view_texture; // 렌더링된 이미지가 저장되는 텍스처
    Vector2 viewport_mouse_pos = { 0.0, 0.0 };
    bool is_viewport_hovered   = false;
}

namespace ImGui
{
    void DrawAxes(float length, float thickness)
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

    void Show3D(const char* title, bool open, std::function<void()> func)
    {
        if (!open)
            return;

        if (!view_texture.id)
            view_texture = LoadRenderTexture((int)WIDTH_3D, (int)HEIGHT_3D);



        ImGui::Begin(title);
        ImGui::BeginChild("##3D Window", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float targetAspect = WIDTH_3D / HEIGHT_3D;
        float availAspect = availSize.x / availSize.y;

        // 렌더링 뷰포트 크기 계산
        ImVec2 imageSize;
        if (availAspect > targetAspect) {
            imageSize.y = availSize.y;
            imageSize.x = imageSize.y * targetAspect;
        } else {
            imageSize.x = availSize.x;
            imageSize.y = imageSize.x / targetAspect;
        }

        // 로컬 오리진 좌표 계산
        ImVec2 cursorStartPos = ImGui::GetCursorScreenPos();
        float offsetX = (availSize.x - imageSize.x) * 0.5f;
        float offsetY = (availSize.y - imageSize.y) * 0.5f;
        ImVec2 imagePos = ImVec2(cursorStartPos.x + offsetX, cursorStartPos.y + offsetY);
        ImGui::SetCursorScreenPos(imagePos);

        // 텍스쳐 렌더링
        ImTextureID tex_id = (ImTextureID)(intptr_t) view_texture.texture.id;
        ImGui::Image(tex_id, imageSize, ImVec2(0, 1), ImVec2(1, 0));

        // 렌더링 영역 클릭 여부 & 클릭 좌표 위치 계산
        is_viewport_hovered = ImGui::IsItemHovered();
        ImVec2 imMousePosGlobal = ImGui::GetMousePos();
        viewport_mouse_pos.x = (imMousePosGlobal.x - imagePos.x) * (WIDTH_3D / imageSize.x);
        viewport_mouse_pos.y = (imMousePosGlobal.y - imagePos.y) * (HEIGHT_3D / imageSize.y);



        BeginTextureMode(view_texture);
        ClearBackground(BLANK);

        // 커서가 숨겨져 있을 때 (카메라 조작 중)
        if (IsCursorHidden()) {
            // [뷰포트 호버 여부와 상관없이 우클릭으로 무조건 커서 복구]
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                EnableCursor();
            }
            // 카메라 업데이트 수행
            UpdateCamera(&ImGui::camera, CAMERA_FREE);
        }
        // 커서가 보일 때 (일반 UI 조작 중)
        else {
            // [뷰포트 위에 있을 때만 우클릭으로 커서 숨기기 (카메라 조작 시작)]
            if (is_viewport_hovered) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    DisableCursor();
                }
            }
        }



        // 렌더링
        BeginMode3D(ImGui::camera);
            DrawGrid(10, 1);        // 그리드
            DrawAxes(0.5, 0.01);    // 월드 축
        EndMode3D();
        DrawFPS(10, 10);
        EndTextureMode();

        ImGui::EndChild();
        ImGui::End();
    }

    void Destroy3D()
    {
        UnloadRenderTexture(view_texture);

    }
}
