#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>

namespace ImGui
{
std::shared_ptr<Texture2D> load_texture(const char* path);

Model load_skybox(const char* vsFileName, const char* fsFileName, const char* cubemapFileName);

}
