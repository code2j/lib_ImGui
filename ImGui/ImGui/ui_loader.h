#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <vector>




namespace ImGui
{
    using Texture = std::weak_ptr<Texture2D>;

    Texture load_texture(const char* path);
    Model   load_skybox(const char* path);

    void destroy_textures();
}
