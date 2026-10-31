#include "ui_loader.h"

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

    void destroy_textures()
    {
        for (const auto& texture : LOADED_TEXTURES) {
            if (texture->id != 0) {
                UnloadTexture(*texture);
                *texture = {};
            }
        }
        LOADED_TEXTURES.clear();
    }
}
