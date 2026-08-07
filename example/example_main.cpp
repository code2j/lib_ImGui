#include "ImGui.hpp"
#include <iostream>


int main() {

    ImGui::init("Demo", 1280, 720);


    while (ImGui::is_running()) {
        ImGui::context([]() {

        });
    }

    ImGui::destroy();

    return 0;
}


