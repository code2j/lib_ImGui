#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "imgui_impl_opengl3_loader.h"

namespace ImGui
{

    inline ImFont* Regular = NULL;
    inline ImFont* Bold = NULL;

    // ------------------------------
    // 기본
    // ------------------------------

    void init(const char* title, const ImVec2& size = ImVec2(1280, 720));
    void destroy();
    void context(std::function<void()> func);
    bool is_running();
    void set_config_path(const std::string& path); // init 호출하기 전에 호출 할 것


    // ------------------------------
    // 이미지
    // ------------------------------
    struct Image_ {
        GLuint texture_id;
        ImVec2 size;

        Image_() : texture_id(0), size(0, 0) {}
        ~Image_();
    };
    std::shared_ptr<Image_> load_image(const std::string& path);
}
