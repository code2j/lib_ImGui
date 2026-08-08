#include "gui.hpp"
#include <iostream>
#include <bits/this_thread_sleep.h>




int main() {
    ImGui::init("테스트 프로그램", 1280, 720);



    while (ImGui::is_running()) {

        ImGui::context([&]() {




        });
    }



    ImGui::destroy();

    return 0;
}


