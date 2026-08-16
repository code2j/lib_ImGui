#pragma once

#include "imgui.h"

namespace ImGui
{
    // ===================================================
    // CollapsingHeader(Animated)
    // ====================================================
    bool BeginCollapsingHeader(const char* label, bool default_open = false);
    void EndCollapsingHeader(const char* label);


}