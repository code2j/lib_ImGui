#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <vector>




namespace ImGui
{
    using Image2D = std::shared_ptr<const Texture2D>;

    Image2D LoadImage(const char* path);

    void DestroyAllTextures();
}
