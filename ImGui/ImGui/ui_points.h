#pragma once
#include "raylib.h"
#include <vector>
#include <eigen3/Eigen/Eigen>

namespace ImGui
{
    struct Points {
        Mesh                    mesh        = GenMeshSphere(0.005, 8, 8);
        std::vector<Matrix>     transforms;
        Material                material    = LoadMaterialDefault();
        std::vector<Vector3>    data;
        Color                   color       = SKYBLUE;

        ~Points();

        void move(const Eigen::Matrix4d& transform);
    };


    bool load_points(const char* path, Points* out);
    void draw_points(const Points& points);
}