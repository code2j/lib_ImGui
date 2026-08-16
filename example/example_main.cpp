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


        if (ImGui::BeginCollapsingHeader(ICON_MD_CHAT_INFO " Notification Example ")) {
            // [알림 버튼 샘플]
            if (ImGui::Button(" 알림 정보 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "정보 알림이 표시 됩니다."));
                std::cout << "[Info ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 성공 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Success, "성공 알림이 표시 됩니다."));
                std::cout << "[Succ ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 경고 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Warning, "경고 알림이 표시 됩니다."));
                std::cout << "[Warn ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 에러 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, "에러 알림이 표시 됩니다."));
                std::cout << "[Error] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
            }

            ImGui::EndCollapsingHeader(ICON_MD_CHAT_INFO " Notification Example ");
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_TUNE " Slider Example ")) {
            // [드래그 슬라이더 샘플]
            ImGui::BeginChild("##slider", ImVec2(0, 190), true);
            ImGui::PushItemWidth(300);

            static float drag = 10.0f;
            ImGui::DragFloat("Drag", &drag);
            ImGui::Dummy(ImVec2(0, 1));

            static float slider = 0.314f;
            ImGui::SliderFloat("Slider", &slider, 0.0f, 1.0f);
            ImGui::Dummy(ImVec2(0, 1));

            // [범위 슬라이더 샘플]
            static float price_min = 0.0f;
            static float price_max = 10.0f;
            ImGui::SliderFloatRange("Range", &price_min, &price_max, 0.0f, 10.0f, "%.1f");
            ImGui::Dummy(ImVec2(0, 1));

            static int cnt = 1.0;
            if (ImGui::InputInt("Input", &cnt)) {
                cnt = std::max(static_cast<int>(price_min), std::min(cnt, static_cast<int>(price_max)));
            }
            ImGui::Dummy(ImVec2(0, 1));


            ImGui::PopItemWidth();
            ImGui::EndChild();

            ImGui::EndCollapsingHeader(ICON_MD_TUNE " Slider Example ");
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_JOYSTICK " Joystick Example ")) {
            // [조이스틱 샘플]
            ImVec2 joy;
            ImGui::joystic(&joy);
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::EndCollapsingHeader(ICON_MD_JOYSTICK " Joystick Example ");
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_STEPPERS " Status Step Example ", false)) {
            const char* status_labels[] = {
                "시스템 시작",
                "시스템 초기화",
                "시스템 준비",
                "시스템 동작",
            };

            static int current_account_status = 2;

            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            ImGui::BeginChild("상태바", ImVec2(0, 200), true);

            // 1. 스테이터스 바 렌더링 (상단 위치)
            ImGui::StatusStepBar("##AccountStatusStepBar", &current_account_status, status_labels, 4);

            ImGui::Dummy(ImVec2(0, 10.0f)); // 바 위젯과의 여백 확보

            // 2. InputInt 위젯 가로 중앙 정렬 처리
            float input_width = 140.0f; // InputInt의 너비 지정
            float avail_width = ImGui::GetContentRegionAvail().x;
            float offset_x = (avail_width - input_width) * 0.5f;

            if (offset_x > 0.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset_x);

            ImGui::PushItemWidth(input_width);
            if (ImGui::InputInt("##상태", &current_account_status)) {
                current_account_status = std::max(0, std::min(current_account_status, 3));
            }
            ImGui::PopItemWidth();

            ImGui::EndChild();
            ImGui::PopStyleVar();

            ImGui::EndCollapsingHeader(ICON_MD_STEPPERS " Status Step Example ");
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_IMAGE " Image Example ")) {
            // 토클 이미지 보이기
            static bool show_image = false;
            if (ImGui::ToggleButton("이미지 보이기", &show_image)) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "이미지가 표시됩니다."));
            }
            ImGui::SameLine();
            ImGui::Text("이미지 보이기");


            ImGui::BeginChild("child", ImVec2(0, 400), true);
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
            ImGui::EndCollapsingHeader(ICON_MD_IMAGE " Image Example ");

        }


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


