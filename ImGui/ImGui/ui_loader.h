#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <vector>




namespace ImGui
{
    // Remains accessible after destroy_textures(), but its id becomes zero.
    using Image2D = std::shared_ptr<const Texture2D>;

    Image2D LoadImage(const char* path);
    Model   load_skybox(const char* path);

    void destroy_textures();
}
