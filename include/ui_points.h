#pragma once
#include "ui_loader.h"

namespace ImGui
{
    struct Points {
        Mesh                    mesh        = GenMeshSphere(0.005, 8, 8);
        std::vector<Matrix>     transforms;
        Material                material    = LoadMaterialDefault();
        std::vector<Vector3>    data;
        Color                   color       = SKYBLUE;
    };


    bool load_points(const char* path, Points* out);
    void draw_points(const Points& points);
}