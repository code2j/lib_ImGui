#include "ui_3d.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "ui.hpp"
#include "ui_icon.h"
#include <vector>
#include <cmath>


namespace {
    const float WIDTH_3D= 1280;
    const float HEIGHT_3D= 720;

    RenderTexture2D view_texture; // 렌더링된 이미지가 저장되는 텍스처
    Vector2 viewport_mouse_pos = { 0.0, 0.0 };
    bool is_viewport_hovered   = false;
    bool camera_control = false;
    bool camera_pan = false;
    bool viewport_clicked = false;
    void StopCameraControl()
    {
        camera_control = false;
        camera_pan = false;
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
        return is_viewport_hovered && !camera_control && !camera_pan;
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
            is_viewport_hovered = false;
            StopCameraControl();
            return;
        }

        if (!view_texture.id)
            view_texture = LoadRenderTexture((int)WIDTH_3D, (int)HEIGHT_3D);



        ImGui::Begin(title);
        ImGui::BeginChild("##3D Window", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float targetAspect = WIDTH_3D / HEIGHT_3D;
        if (availSize.x <= 0.0f || availSize.y <= 0.0f) {
            is_viewport_hovered = false;
            StopCameraControl();
            ImGui::EndChild();
            ImGui::End();
            return;
        }
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

        // 좌클릭을 뷰포트에서 시작한 경우에만 대상을 중심으로 회전한다.
        if (is_viewport_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            camera_control = true;
        if (camera_control) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !IsWindowFocused()) {
                StopCameraControl();
            } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                const ImVec2 delta = ImGui::GetIO().MouseDelta;
                const Vector3 offset = Vector3Subtract(camera.position, camera.target);
                const float distance = Vector3Length(offset);
                if (distance > 0.0001f) {
                    float yaw = std::atan2(offset.x, offset.z) - delta.x * 0.005f;
                    float pitch = std::asin(Clamp(offset.y / distance, -1.0f, 1.0f)) + delta.y * 0.005f;
                    pitch = Clamp(pitch, -1.5533f, 1.5533f);
                    const float horizontal = distance * std::cos(pitch);
                    camera.position = Vector3Add(camera.target, Vector3{
                        horizontal * std::sin(yaw), distance * std::sin(pitch), horizontal * std::cos(yaw)});
                }
            }
        }

        // 휠은 뷰포인트와 대상 사이의 거리를 조절한다.
        const float wheel = ImGui::GetIO().MouseWheel;
        if (is_viewport_hovered && wheel != 0.0f && IsWindowFocused()) {
            if (camera.projection == CAMERA_ORTHOGRAPHIC) {
                camera.fovy = Clamp(camera.fovy * std::exp(-wheel * 0.15f), 0.01f, 10000.0f);
            } else {
                const Vector3 offset = Vector3Subtract(camera.position, camera.target);
                const float distance = Vector3Length(offset);
                if (distance > 0.0001f) {
                    const float zoomed = Clamp(distance * std::exp(-wheel * 0.15f), 0.05f, 10000.0f);
                    camera.position = Vector3Add(camera.target, Vector3Scale(offset, zoomed / distance));
                }
            }
        }

        // 우클릭 드래그는 카메라와 대상점을 함께 XZ 평면에서 이동한다.
        if (is_viewport_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            camera_pan = true;
        if (camera_pan) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) || !IsWindowFocused()) {
                camera_pan = false;
            } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
                const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));
                const float distance = Vector3Distance(camera.position, camera.target);
                const float view_height = camera.projection == CAMERA_ORTHOGRAPHIC ? camera.fovy :
                    2.0f * distance * std::tan(camera.fovy * DEG2RAD * 0.5f);
                const float units_per_pixel = view_height / imageSize.y;
                const ImVec2 delta = ImGui::GetIO().MouseDelta;
                Vector3 movement = Vector3Scale(Vector3Add(Vector3Scale(right, -delta.x),
                                                           Vector3Scale(up, delta.y)), units_per_pixel);
                movement.y = 0.0f;
                camera.position = Vector3Add(camera.position, movement);
                camera.target = Vector3Add(camera.target, movement);
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
