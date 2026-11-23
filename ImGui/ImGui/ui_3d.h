#pragma once
#include "imgui.h"
#include "raylib.h"
#include <functional>
#include <string>
#include <unordered_map>

// Commands are submitted each frame from the Context UI callback.
namespace ImGui3D
{
    // Models are owned by ImGui3D; groups allow an entire scene to be replaced.
    class ModelGroup {
    public:
        ModelGroup();
        ~ModelGroup();
        ModelGroup(const ModelGroup&) = delete;
        ModelGroup& operator=(const ModelGroup&) = delete;
        Model* Load(const std::string& path);
        Model* Find(const std::string& path);
        void Clear();
        void Swap(ModelGroup& other);
    private:
        std::unordered_map<std::string, Model> models_;
    };
    enum class Primitive { Box, Sphere, Cylinder };
    Model& GetPrimitive(Primitive primitive);

    struct PrincipledMaterial {
        float base[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        float metallic = 0, roughness = 0.5f, ior = 1.5f, specular_ior_level = 0.5f;
    };
    void DestroyShader();
    void SetMaterial(const PrincipledMaterial* material = nullptr);

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

    // Call EndScene only when BeginScene returns true. Models and their textures
    // must remain valid until EndScene has replayed the recorded draws.
    bool BeginScene(const char* title, bool open, bool block_viewport_input = false);
    void EndScene();
    bool Get3DClick(Vector2& position);
    bool Get3DMouse(Vector2& position);
    void Destroy3D();
}
