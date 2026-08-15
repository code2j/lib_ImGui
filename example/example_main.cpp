#include "ui.hpp"
#include <iostream>





int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");





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
        }));
    } // 스코프 해제 시점


    ImGui::destroy();
    return 0;
}


