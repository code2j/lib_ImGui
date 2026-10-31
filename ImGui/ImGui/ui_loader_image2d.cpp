#include "ui_loader.h"
#include <limits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace ImGui
{
    static std::vector<std::shared_ptr<Texture2D>> LOADED_TEXTURES;

    Image2D LoadImage(const char* path)
    {
        Texture2D tex = LoadTexture(path);
        if (tex.id == 0) return Image2D();

        std::shared_ptr<Texture2D> ptr(
            new Texture2D(tex),
            [](Texture2D* p) {
                if (p) {
                    if (p->id > 0) UnloadTexture(*p);
                    delete p;
                }
            }
        );

        LOADED_TEXTURES.push_back(ptr);

        return ptr;
    }

    static bool UpdateImageTexture(Image2D& image, const void* data, size_t data_size, int width, int height, size_t stride, PixelFormat format, bool big_endian)
    {
        // 입력 픽셀 형식과 채널 수 확인
        const bool mono = format == PixelFormat::Mono8;
        const bool bgr = format == PixelFormat::BGR8 || format == PixelFormat::BGRA8;
        const bool depth16 = format == PixelFormat::Depth16;
        const bool depth32 = format == PixelFormat::Depth32F;
        const size_t channels = depth16 ? 2 : (depth32 ? 4 : (mono ? 1 : (format == PixelFormat::RGBA8 || format == PixelFormat::BGRA8 ? 4 : 3)));

        // 이미지 크기, 행 크기, 데이터 크기 검사
        if (!data || width <= 0 || height <= 0 ||
            static_cast<size_t>(width) > stride / channels ||
            static_cast<size_t>(height) > data_size / stride ||
            static_cast<size_t>(width) > std::numeric_limits<size_t>::max() / 4 / static_cast<size_t>(height))
            return false;

        // 입력 픽셀을 RGBA 형식으로 변환
        std::vector<unsigned char> rgba(static_cast<size_t>(width) * height * 4);
        for (int y = 0; y < height; ++y) {
            const auto* src = static_cast<const unsigned char*>(data) + static_cast<size_t>(y) * stride;
            auto* dst = rgba.data() + static_cast<size_t>(y) * width * 4;
            for (int x = 0; x < width; ++x, src += channels, dst += 4) {
                if (depth16 || depth32) {
                    // 깊이 0~5m를 흑백으로 변환, 유효하지 않은 값은 검정
                    uint32_t bits = 0;
                    for (size_t i = 0; i < channels; ++i)
                        bits = (bits << 8) | src[big_endian ? i : channels - 1 - i];
                    float meters;
                    if (depth16) meters = static_cast<float>(bits) * 0.001f;
                    else std::memcpy(&meters, &bits, sizeof(meters));
                    const auto gray = std::isfinite(meters) && meters > 0.0f
                        ? static_cast<unsigned char>(std::min(meters / 5.0f, 1.0f) * 255.0f) : 0;
                    dst[0] = dst[1] = dst[2] = gray;
                    dst[3] = 255;
                } else {
                    dst[0] = src[mono ? 0 : (bgr ? 2 : 0)];
                    dst[1] = src[mono ? 0 : 1];
                    dst[2] = src[mono ? 0 : (bgr ? 0 : 2)];
                    dst[3] = channels == 4 ? src[3] : 255;
                }
            }
        }

        // 이미지에 해당하는 관리 중인 텍스처 찾기
        std::shared_ptr<Texture2D> texture;
        for (const auto& loaded : LOADED_TEXTURES) {
            if (loaded.get() == image.get()) {
                texture = loaded; break;
            }
        }

        // 기존 텍스처의 크기와 형식이 같으면 픽셀만 업데이트
        if (texture && texture->id && texture->width == width && texture->height == height &&
            texture->format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
            UpdateTexture(*texture, rgba.data());
            return true;
        }

        // 변환된 픽셀을 참조하는 Image 객체 생성
        // 이미지 크기가 바뀐 경우, 새로운 Image 객체를 생성
        ::Image frame;
        frame.data = rgba.data();
        frame.width = width;
        frame.height = height;
        frame.mipmaps = 1;
        frame.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        // 새 텍스처를 생성하고 픽셀을 GPU에 업로드
        Texture2D replacement = LoadTextureFromImage(frame);
        if (!replacement.id)
            return false;

        if (texture) {
            // 기존 GPU 텍스처를 해제하고 새 텍스처로 교체
            if (texture->id) UnloadTexture(*texture);
            *texture = replacement;
        }
        else {
            // 새 텍스처를 등록하고 마지막 참조가 해제될 때 정리
            texture = std::shared_ptr<Texture2D>(new Texture2D(replacement), [](Texture2D* p) {
                if (p->id) UnloadTexture(*p);
                delete p;
            });
            LOADED_TEXTURES.push_back(texture);
            image = texture;
        }

        return true;
    }

    static std::unordered_map<std::string, Image2D> NAMED_IMAGES;

    static bool StoreImage(const char* name, const void* data, size_t data_size, int width, int height, size_t stride, PixelFormat format, bool big_endian)
    {
        if (!name || !*name)
            return false;

        // 이미지 이름이 있으면 이미지 업데이트
        auto it = NAMED_IMAGES.find(name);
        if (it != NAMED_IMAGES.end())
            return UpdateImageTexture(it->second, data, data_size, width, height, stride, format, big_endian);

        // 이미지 이름이 없으면 새로운 이미지 생성
        Image2D image;
        if (!UpdateImageTexture(image, data, data_size, width, height, stride, format, big_endian))
            return false;

        // 전역 관리 변수에 추가
        NAMED_IMAGES.emplace(name, std::move(image));
        return true;
    }

    bool Image(const char* name, const void* data, size_t data_size, int width, int height, size_t stride, PixelFormat format, ImVec2 size, bool big_endian)
    {
        if (size.x <= 0 || size.y <= 0 ||
            !StoreImage(name, data, data_size, width, height, stride, format, big_endian)) return false;
        const auto& image = NAMED_IMAGES.at(name);
        const float scale = std::min(size.x / image->width, size.y / image->height);
        ImGui::Image(image->id, ImVec2(image->width * scale, image->height * scale));
        return true;
    }

    void RemoveImage(const char* name)
    {
        if (!name) return;
        auto it = NAMED_IMAGES.find(name);
        if (it == NAMED_IMAGES.end()) return;
        auto texture = it->second;
        NAMED_IMAGES.erase(it);
        auto loaded = std::find_if(LOADED_TEXTURES.begin(), LOADED_TEXTURES.end(),
            [&](const auto& item) { return item.get() == texture.get(); });
        if (loaded != LOADED_TEXTURES.end()) {
            if ((*loaded)->id) UnloadTexture(**loaded);
            **loaded = {};
            LOADED_TEXTURES.erase(loaded);
        }
    }

    void DestroyAllTextures()
    {
        for (const auto& texture : LOADED_TEXTURES) {
            if (texture->id != 0) {
                UnloadTexture(*texture);
                *texture = {};
            }
        }
        LOADED_TEXTURES.clear();
        NAMED_IMAGES.clear();
    }
}
