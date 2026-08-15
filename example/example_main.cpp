#include "ui.hpp"
#include <iostream>


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>




// -------------------------------------------------------------
// PLY 파싱 및 GPU Mesh 생성 함수
// -------------------------------------------------------------



int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");

    const std::string ply_file = "/home/jusik/workspace/lib_ImGui/aaa.ply";

    ImGui::Points points;
    if (!ImGui::load_points(ply_file.c_str(), &points)) {
        std::cout << "[Warn ] [Main] 데이터 불러오기 실패: " << ply_file << std::endl;
        return 1;
    }

    float pointSize = 0.01f;
    Mesh pointMesh = GenMeshSphere(0.005, 10, 10);

    Shader instancingShader = LoadShader("/home/jusik/workspace/lib_ImGui/data/shaders/lighting_instancing.vs",
                                         "/home/jusik/workspace/lib_ImGui/data/shaders/lighting.fs");

    int ambientLoc = GetShaderLocation(instancingShader, "ambient");
    float ambient[4] = { 10.0f, 10.0f, 10.0f, 10.0f }; // R, G, B, Alpha 순서
    SetShaderValue(instancingShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

    Material pointMat = LoadMaterialDefault();
    pointMat.shader = instancingShader; // <--- 이 부분이 핵심입니다!
    pointMat.maps[MATERIAL_MAP_DIFFUSE].color = SKYBLUE;


    Matrix* transforms = nullptr;
    if (!points.empty()) {
        transforms = (Matrix*)MemAlloc(points.size() * sizeof(Matrix));
        for (size_t i = 0; i < points.size(); i++) {
            transforms[i] = MatrixTranslate(points[i].x, points[i].y, points[i].z);
        }
    }

    {   // 스코프 안에서 생성하면 자동으로 해제됨
        ImGui::Texture texture1 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO1.png");
        ImGui::Texture texture2 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO2.png");
        ImGui::Texture texture3 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO3.png");


        while (ImGui::context([&]() {
            ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

            // 조이스틱
            ImVec2 joy;
            if (ImGui::joystic(" " ICON_MD_JOYSTICK " 조이스틱 ", &joy)) {
                std::cout << "[Info ] 조이스틱 값: (" << joy.x << ", " << joy.y << ")" << std::endl;
            }
            ImGui::Dummy(ImVec2(0, 20));

            // 토클 이미지 보이기
            static bool show_image = false;
            if (ImGui::ToggleButton("이미지 보이기", &show_image)) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "이미지가 표시됩니다."));
                std::cout << "cout으로 출력된 메세지는 로그에 나타납니다." << std::endl;
            }
            ImGui::SameLine();
            ImGui::Text("이미지 보이기");


            ImGui::BeginChild("child", ImVec2(0, 0), true);
            if (show_image) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float img_aspect = (float) texture1->width / (float) texture1->height;
                float avail_aspect = avail.x / avail.y;

                ImVec2 size;
                if (avail_aspect > img_aspect) {
                    // 가용 영역이 더 넓음 - 높이에 맞춤
                    size.y = avail.y;
                    size.x = size.y * img_aspect;
                } else {
                    // 가용 영역이 더 좁음 - 너비에 맞춤
                    size.x = avail.x;
                    size.y = size.x / img_aspect;
                }

                ImGui::Image(texture1->id, size);
                ImGui::Image(texture2->id, size);
                ImGui::Image(texture3->id, size);
            }
            ImGui::EndChild();


            ImGui::End();

            ImGui::ShowDemoWindow();


            if (transforms != nullptr) {
                DrawMeshInstanced(pointMesh, pointMat, transforms, points.size());
            }
        }));
    } // 스코프 해제 시점


    ImGui::destroy();
    return 0;
}


