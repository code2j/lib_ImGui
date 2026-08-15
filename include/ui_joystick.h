#pragma once
#include "imgui.h"
#include "eigen3/Eigen/Dense"

namespace ImGui
{
    bool joystic(const char* title, ImVec2* out);
    bool tf_control(Eigen::Matrix4d* matrix);
}
