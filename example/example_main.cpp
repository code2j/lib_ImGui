#include "gui.hpp"
#include <iostream>






int main() {
    ImGui::init("ImGui 예제", 1280, 720);


    std::cout << "Application Started!" << std::endl;
    std::cout << "Waiting for user input..." << std::endl;

    while (ImGui::is_running()) {

        if (IsKeyPressed(KEY_SPACE)) {
            std::cout << "Spacebar pressed! Time: " << GetTime() << std::endl;
        }



        ImGui::context([&]() {

            if (ImGui::Button("Test")) {
                ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "Hello World! This is a success! %s", "We can also format here:)" });
            }



        });
    }

    ImGui::destroy();

    return 0;
}


