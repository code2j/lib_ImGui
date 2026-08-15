#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();

bool DrawTfControlWidget(Eigen::Matrix4d* matrix)
{
    // 유효하지 않은 포인터가 들어오면 바로 종료하여 크래시 방지
    if (!matrix) {
        return false;
    }

    bool is_changed = false;

    // 위젯 고유 ID 부여 (matrix의 메모리 주소를 그대로 사용하여 충돌 방지)
    ImGui::PushID(matrix);

    // 위젯 내부에서 상태를 유지할 변수
    static float translation[3] = { 0.0f, 0.0f, 0.0f };
    static float rotation_deg[3] = { 0.0f, 0.0f, 0.0f };

    // --- [1] 초기화 버튼 ---
    if (ImGui::Button("Reset All", ImVec2(-1, 0))) {
        for(int i = 0; i < 3; ++i) {
            translation[i] = 0.0f;
            rotation_deg[i] = 0.0f;
        }
        is_changed = true;
    }

    ImGui::Spacing();

    // --- [2] 이동 (Translation) 컨트롤 ---
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Translation (X, Y, Z)");
    ImGui::PushItemWidth(-1);
    if (ImGui::DragFloat3("##Trans", translation, 0.01f, 0.0f, 0.0f, "%.3f")) {
        is_changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();

    // --- [3] 회전 (Rotation) 컨트롤 ---
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Rotation [Deg] (Roll, Pitch, Yaw)");
    ImGui::PushItemWidth(-1);
    if (ImGui::DragFloat3("##Rot", rotation_deg, 0.5f, 0.0f, 0.0f, "%.1f")) {
        is_changed = true;
    }
    ImGui::PopItemWidth();

    // --- [4] 변경 시 Matrix 업데이트 ---
    if (is_changed) {
        double roll_rad  = rotation_deg[0] * (M_PI / 180.0);
        double pitch_rad = rotation_deg[1] * (M_PI / 180.0);
        double yaw_rad   = rotation_deg[2] * (M_PI / 180.0);

        Eigen::AngleAxisd rollAngle(roll_rad, Eigen::Vector3d::UnitX());
        Eigen::AngleAxisd pitchAngle(pitch_rad, Eigen::Vector3d::UnitY());
        Eigen::AngleAxisd yawAngle(yaw_rad, Eigen::Vector3d::UnitZ());

        Eigen::Matrix3d rotation_matrix = (yawAngle * pitchAngle * rollAngle).matrix();

        // 포인터 접근(->)을 통한 행렬 조작
        matrix->setIdentity();
        matrix->block<3, 3>(0, 0) = rotation_matrix;
        matrix->block<3, 1>(0, 3) = Eigen::Vector3d(translation[0], translation[1], translation[2]);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- [5] 결과 행렬 출력 ---
    ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Current Matrix (4x4)");
    if (ImGui::BeginTable("MatrixTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
        for (int i = 0; i < 4; ++i) {
            ImGui::TableNextRow();
            for (int j = 0; j < 4; ++j) {
                ImGui::TableSetColumnIndex(j);
                // 괄호와 역참조 연산자(*matrix)를 사용하여 요소 접근
                if (j == 3 && i != 3) {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%8.3f", (*matrix)(i, j));
                } else {
                    ImGui::Text("%8.3f", (*matrix)(i, j));
                }
            }
        }
        ImGui::EndTable();
    }

    ImGui::PopID(); // 위젯 ID 종료

    return is_changed; // 값이 변한 프레임에만 true 리턴
}

int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");

    ImGui::Points points;
    if (!ImGui::load_points(FILE_PCD, &points)) {
        std::cout << "[Warn ] [Main] 데이터 불러오기 실패: " << FILE_PCD << std::endl;
        return 1;
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

            ImGui::Begin(" " ICON_MD_GAMEPAD " TF 컨트롤 ");
            DrawTfControlWidget(&tf_control);
            ImGui::End();

            points.move(tf_control);
            ImGui::draw_points(points);
        }));
    } // 스코프 해제 시점


    ImGui::destroy();
    return 0;
}


