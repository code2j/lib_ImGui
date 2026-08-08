#include "gui.hpp"
#include <iostream>
#include <bits/this_thread_sleep.h>




int main() {
    ImGui::init("ImGui 예제", 1280, 720);



    while (ImGui::is_running()) {

        ImGui::context([&]() {



            ImGui::Begin("테스트 창");
                if (ImGui::Button("Test")) {
                    ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "메세지 테스트 한다잇" });
                }
                if (ImGui::Button("Test1")) {
                   ImGui::InsertNotification({ ImGuiToastType_Warning, 3000, "Hello World! This is a warning! %d", 0x1337 });          }
                if (ImGui::Button("Test2")) {
                   ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Hello World! This is an error! 0x%X", 0xDEADBEEF });          }
                if (ImGui::Button("Test3")) {
                   ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Hello World! This is an info!" });
                }

                static bool is_test_button_pressed = false;
                ImGui::ToggleButton("테스트 버튼", &is_test_button_pressed   );
            ImGui::End();



        });
    }



    ImGui::destroy();

    return 0;
}


