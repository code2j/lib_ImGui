#include "ui_loader.h"


namespace ImGui
{

std::shared_ptr<Texture2D> load_texture(const char* path)
{
    Texture2D tex = LoadTexture(path);
    if (tex.id == 0) return nullptr;

    // shared_ptr가 소멸될 때 UnloadTexture 자동 호출
    return std::shared_ptr<Texture2D>(
        new Texture2D(tex),
        [](Texture2D* ptr) {
            if (ptr) {
                if (ptr->id > 0) UnloadTexture(*ptr);
                delete ptr;
            }
        }
    );
}

}

