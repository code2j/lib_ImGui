#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <cstddef>
#include <vector>



using ImImage = std::shared_ptr<const Texture2D>;

namespace ImGui
{

    ImImage LoadImage(const char* path);

    enum class PixelFormat { RGB8, BGR8, RGBA8, BGRA8, Mono8, Depth16, Depth32F };
    bool Pixel2Image(const char* name, const void* data, size_t data_size, int width, int height, size_t stride, PixelFormat format, ImVec2 size, bool big_endian = false);
    void RemoveImage(const char* name);

    void DestroyAllTextures();
}
