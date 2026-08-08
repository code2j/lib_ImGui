#include "gui.hpp"
#include <iostream>




int main() {
    ImGui::init("테스트 프로그램", 1280, 720);


    std::shared_ptr<Texture2D> texture1;
    std::shared_ptr<Texture2D> texture2;
    std::shared_ptr<Texture2D> texture3;




    if (!texture1)
        texture1 = ImGui::load_texture(IMGUI_ROOT "data/RAKOKO1.png");
    if (!texture2)
        texture2 = ImGui::load_texture(IMGUI_ROOT "data/RAKOKO2.png");
    if (!texture3)
        texture3 = ImGui::load_texture(IMGUI_ROOT "data/RAKOKO3.png");




    while (ImGui::is_running()) {


        ImGui::context([&]() {

            ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");


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
        });
    }



    ImGui::destroy();

    return 0;
}


