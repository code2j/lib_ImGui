#pragma once
#include "imgui.h"
#include <iostream>


class ImGuiLogWindow : public std::streambuf {
private:
    ImGuiTextBuffer buf;
    bool auto_scroll;
    std::streambuf* old_buf;

    ImFont* D2Cording = nullptr;
protected:
    virtual int overflow(int c) override;
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override;



public:
    ImGuiLogWindow();
    ~ImGuiLogWindow();

    void clear();

    void load_font();

    void draw(const char* title, bool* p_open = nullptr);
};