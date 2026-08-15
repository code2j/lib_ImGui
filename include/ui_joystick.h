#pragma once
#include "imgui.h"
#include "eigen3/Eigen/Dense"

namespace ImGui
{
    bool joystic(const char* title, ImVec2* out);
    bool tf_widget(Eigen::Matrix4d* matrix);
}
