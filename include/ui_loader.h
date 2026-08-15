#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <vector>




namespace ImGui
{
    using Texture   = std::shared_ptr<Texture2D>;
    using Points    = std::vector<Vector3>;


    std::shared_ptr<Texture2D>  load_texture(const char* path);
    Model                       load_skybox(const char* path);
    bool                        load_points(const char* path, Points* out);
}
