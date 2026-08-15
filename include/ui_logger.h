#pragma once
#include "imgui.h"
#include <iostream>


class ImGuiLogger : public std::streambuf {
public:
    ImGuiLogger();
    ~ImGuiLogger();

    void clear();
    void draw(const char* title, bool* p_open = nullptr);

protected:
    virtual int overflow(int c) override;
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
};