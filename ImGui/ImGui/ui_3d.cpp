#include "ui_3d.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "ui.hpp"
#include "ui_font_icon.h"
#include <vector>


namespace {
    const float WIDTH_3D= 1280;
    const float HEIGHT_3D= 720;

    RenderTexture2D view_texture; // 렌더링된 이미지가 저장되는 텍스처
    Vector2 viewport_mouse_pos = { 0.0, 0.0 };
    bool is_viewport_hovered   = false;
    bool camera_control = false;
    bool viewport_clicked = false;
    bool restore_no_mouse = false;
    Vector2 saved_mouse_pos = { 0.0f, 0.0f };

    void StopCameraControl()
    {
        if (!camera_control) return;
        EnableCursor();
        SetMousePosition((int)saved_mouse_pos.x, (int)saved_mouse_pos.y);
        if (!restore_no_mouse)
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        camera_control = false;
    }

    enum class Shape { Cube, Sphere, Line };
    struct DrawCommand {
        Shape shape;
        Vector3 first;
        Vector3 second;
        float radius;
        Color color;
    };
    std::vector<DrawCommand> commands;
}

namespace ImGui3D
{
    bool Get3DClick(Vector2& position)
    {
        position = viewport_mouse_pos;
        return viewport_clicked;
    }

    bool Get3DMouse(Vector2& position)
    {
        position = viewport_mouse_pos;
        return is_viewport_hovered && !camera_control;
    }

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
        viewport_clicked = false;
        if (!open) {
            StopCameraControl();
            return;
        }

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
        viewport_clicked = is_viewport_hovered && !camera_control && ImGui::IsMouseClicked(ImGuiMouseButton_Left);



        BeginTextureMode(view_texture);
        ClearBackground(BLANK);

        // 커서가 숨겨져 있을 때 (카메라 조작 중)
        if (camera_control) {
            // 버튼을 놓거나 창 포커스를 잃으면 카메라 조작을 종료한다.
            if (!IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || !IsWindowFocused()) {
                StopCameraControl();
            }
            // 카메라 업데이트 수행
            else {
                const Vector2 delta = GetMouseDelta();
                UpdateCameraPro(&ImGui3D::camera, {0, 0, 0},
                    { delta.x * 0.05f, delta.y * 0.05f, 0.0f }, 0.0f);
                const Vector3 forward = Vector3Normalize(Vector3Subtract(ImGui3D::camera.target, ImGui3D::camera.position));
                const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, ImGui3D::camera.up));
                const float speed = 5.4f * GetFrameTime();
                const Vector3 movement = Vector3Add(
                    Vector3Scale(forward, (IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * speed),
                    Vector3Scale(right, (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * speed));
                ImGui3D::camera.position = Vector3Add(ImGui3D::camera.position, movement);
                ImGui3D::camera.target = Vector3Add(ImGui3D::camera.target, movement);
            }
        }
        // 커서가 보일 때 (일반 UI 조작 중)
        else {
            // [뷰포트 위에 있을 때만 가운데 버튼으로 커서 숨기기 (카메라 조작 시작)]
            if (is_viewport_hovered) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                    saved_mouse_pos = GetMousePosition();
                    restore_no_mouse = (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) != 0;
                    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
                    camera_control = true;
                    DisableCursor();
                }
            }
        }



        // 렌더링
        BeginMode3D(ImGui3D::camera);
            DrawGrid(10, 1);        // 그리드
            DrawAxes(0.1, 0.01);    // 월드 축
            if (func) func();
            ImGui3D::RenderFrame();
        EndMode3D();
        DrawFPS(10, 10);
        EndTextureMode();

        ImGui::EndChild();
        ImGui::End();
    }

    void Destroy3D()
    {
        StopCameraControl();
        UnloadRenderTexture(view_texture);

    }


    void Cube(Vector3 position, Vector3 size, Color color)
    {
        commands.push_back({Shape::Cube, position, size, 0.0f, color});
    }

    void Sphere(Vector3 center, float radius, Color color)
    {
        commands.push_back({Shape::Sphere, center, {}, radius, color});
    }

    void Line(Vector3 start, Vector3 end, Color color)
    {
        commands.push_back({Shape::Line, start, end, 0.0f, color});
    }

    void ClearFrame()
    {
        commands.clear();
    }

    void RenderFrame()
    {
        for (const auto& command : commands) {
            switch (command.shape) {
                case Shape::Cube: DrawCubeV(command.first, command.second, command.color); break;
                case Shape::Sphere: DrawSphere(command.first, command.radius, command.color); break;
                case Shape::Line: DrawLine3D(command.first, command.second, command.color); break;
            }
        }
    }
}
