#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();



int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");

    {

    // 스코프 안에서 생성하면 자동으로 해제됨
    ImGui::Texture texture1 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO1.png");
    ImGui::Texture texture2 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO2.png");
    ImGui::Texture texture3 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO3.png");


    // 포인트 클라우드 데이터 불러오기
    ImGui::Points points;
    if (!ImGui::load_points(FILE_PCD, &points)) {
        std::cout << "[Warn ] [Main] 데이터 불러오기 실패: " << FILE_PCD << std::endl;
        return 1;
    }



    while (ImGui::context([&]() {
        ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

        static float slider = 0.0f;
        ImGui::DragFloat("드레그", &slider);
        ImGui::SliderFloat("슬라이더", &slider, 0.0f, 1.0f);

        if (ImGui::Button("알림1")) {
            ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "정보 알림이 표시 됩니다."));
        }
        if (ImGui::Button("알림2")) {
            ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Success, "성공 알림이 표시 됩니다."));
        }
        if (ImGui::Button("알림3")) {
            ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Warning, "경고 알림이 표시 됩니다."));
        }
        if (ImGui::Button("알림4")) {
            ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, "에러 알림이 표시 됩니다."));
        }



        // 조이스틱
        ImVec2 joy;
        ImGui::joystic(" " ICON_MD_JOYSTICK " 조이스틱 ", &joy);
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

        ImGui::Begin(" " ICON_MD_GAMEPAD " TF 컨트롤 ");
        ImGui::tf_widget(&tf_control);
        ImGui::End();

        points.move(tf_control);
        ImGui::draw_points(points);
    }));

    }


    ImGui::destroy();
    return 0;
}


