#include "ui.hpp"
#include <iostream>



Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();

void draw_tf_control() {
    // ImGui에서 값을 입력받기 위해 상태를 유지할 static 변수 선언
    // xyz: x, y, z 이동 거리
    static float translation[3] = { 0.0f, 0.0f, 0.0f };
    // rpy: Roll(X), Pitch(Y), Yaw(Z) 회전 각도 (단위: Degree)
    static float rotation_deg[3] = { 0.0f, 0.0f, 0.0f };

    ImGui::Begin("TF 제어");

    bool is_changed = false;

    // x, y, z 이동 UI (드래그하여 세밀하게 조절 가능)
    if (ImGui::DragFloat3("Translation (X,Y,Z)", translation, 0.01f)) {
        is_changed = true;
    }

    // roll, pitch, yaw 회전 UI (단위: Degree, 드래그 조절)
    if (ImGui::DragFloat3("Rotation (R,P,Y)", rotation_deg, 1.0f)) {
        is_changed = true;
    }

    // 값 초기화 버튼 추가
    if (ImGui::Button("Reset")) {
        for(int i=0; i<3; ++i) {
            translation[i] = 0.0f;
            rotation_deg[i] = 0.0f;
        }
        is_changed = true;
    }

    // UI에서 값이 변경되었을 때만 매트릭스 재계산
    if (is_changed) {
        // Degree -> Radian 변환
        double roll_rad  = rotation_deg[0] * (M_PI / 180.0);
        double pitch_rad = rotation_deg[1] * (M_PI / 180.0);
        double yaw_rad   = rotation_deg[2] * (M_PI / 180.0);

        // 각 축에 대한 회전 행렬 생성
        Eigen::AngleAxisd rollAngle(roll_rad, Eigen::Vector3d::UnitX());
        Eigen::AngleAxisd pitchAngle(pitch_rad, Eigen::Vector3d::UnitY());
        Eigen::AngleAxisd yawAngle(yaw_rad, Eigen::Vector3d::UnitZ());

        // 회전 행렬 적용 (Yaw * Pitch * Roll 순서가 일반적인 오일러 각 적용 순서)
        Eigen::Matrix3d rotation_matrix = (yawAngle * pitchAngle * rollAngle).matrix();

        // tf_control 행렬 업데이트
        tf_control.setIdentity(); // 매트릭스를 단위 행렬로 초기화

        // 1. 회전(Rotation) 성분 세팅 (좌상단 3x3)
        tf_control.block<3, 3>(0, 0) = rotation_matrix;

        // 2. 이동(Translation) 성분 세팅 (우측 3x1)
        tf_control.block<3, 1>(0, 3) = Eigen::Vector3d(translation[0], translation[1], translation[2]);
    }

    // (선택 사항) 결과 매트릭스를 UI 하단에 텍스트로 출력하여 확인
    ImGui::Separator();
    ImGui::Text("Current Matrix (tf_control):");
    for (int i = 0; i < 4; ++i) {
        ImGui::Text("%8.3f %8.3f %8.3f %8.3f",
            tf_control(i, 0), tf_control(i, 1),
            tf_control(i, 2), tf_control(i, 3));
    }

    ImGui::End();
}


int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");

    const char* ply_file = "/home/jusik/workspace/lib_ImGui/aaa.ply";

    ImGui::Points points;
    if (!ImGui::load_points(ply_file, &points)) {
        std::cout << "[Warn ] [Main] 데이터 불러오기 실패: " << ply_file << std::endl;
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

            draw_tf_control();

            points.move(tf_control);
            ImGui::draw_points(points);
        }));
    } // 스코프 해제 시점


    ImGui::destroy();
    return 0;
}


