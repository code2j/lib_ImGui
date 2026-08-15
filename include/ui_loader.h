#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>



namespace ImGui
{
    using Texture = std::shared_ptr<Texture2D>;


    std::shared_ptr<Texture2D>  load_texture(const char* path);
    Model                       load_skybox(const char* cubemapFileName);
}
