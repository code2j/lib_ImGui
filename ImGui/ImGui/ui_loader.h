#pragma once
#include "imgui.h"
#include "raylib.h"
#include <memory>
#include <cstddef>
#include <vector>




namespace ImGui
{
    using Image2D = std::shared_ptr<const Texture2D>;

    Image2D LoadImage(const char* path);

    enum class PixelFormat { RGB8, BGR8, RGBA8, BGRA8, Mono8, Depth16, Depth32F };
    // 깊이 형식은 16비트 mm 또는 32비트 float m, 표시 범위는 0~5m.
    bool Image(const char* name, const void* data, size_t data_size, int width, int height, size_t stride, PixelFormat format, ImVec2 size, bool big_endian = false);
    void RemoveImage(const char* name);

    void DestroyAllTextures();
}
