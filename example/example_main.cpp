#include "ui.hpp"
#include <iostream>


void joystic_controller(float* out_x = nullptr, float* out_y = nullptr)
{
// 1. 패드 기본 설정
    ImVec2 pad_size(200.0f, 200.0f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 상호작용용 InvisibleButton
    ImGui::InvisibleButton("joystick_pad_3dof", pad_size);
    bool is_active = ImGui::IsItemActive();
    bool is_activated = ImGui::IsItemActivated();

    ImVec2 center = ImVec2(p.x + pad_size.x * 0.5f, p.y + pad_size.y * 0.5f);

    // 반지름 정의
    float outer_ring_radius = pad_size.x * 0.5f - 5.0f;  // Yaw 링 외각
    float inner_ring_radius = outer_ring_radius - 20.0f; // Yaw 링 내각
    float move_radius = inner_ring_radius - 2.0f;        // XY 이동 제한 영역
    float handle_radius = 12.0f;                         // 중앙 작은 원(핸들)의 클릭 히트박스 반지름

    ImVec2 handle_pos = center;
    float output_x = 0.0f;
    float output_y = 0.0f;

    // 조작 상태 및 초기 마우스 클릭 위치
    static bool is_dragging_handle = false;
    static ImVec2 drag_start_mouse_pos = ImVec2(0.0f, 0.0f);

    // -------------------------------------------------------------
    // 클릭 시작 시: 중앙 작은 원(핸들) 안을 눌렀는지 검사
    // -------------------------------------------------------------
    if (is_activated) {
        ImVec2 m_pos = ImGui::GetIO().MousePos;

        // 클릭한 위치와 중앙 핸들(center) 간의 거리 계산
        float dist_to_handle = sqrtf((m_pos.x - center.x) * (m_pos.x - center.x) +
                                     (m_pos.y - center.y) * (m_pos.y - center.y));

        // 핸들 영역(작은 원) 내부를 클릭했을 때만 드래그 허용
        if (dist_to_handle <= handle_radius + 3.0f) { // 약간의 판정 여유(+3.0f) 부여
            is_dragging_handle = true;
            drag_start_mouse_pos = m_pos; // 첫 클릭 위치 기록 (상대 이동용)
        } else {
            is_dragging_handle = false;
        }
    }

    // 마우스를 떼면 상태 초기화 (중앙 복귀)
    if (!is_active) {
        is_dragging_handle = false;
    }

    // -------------------------------------------------------------
    // 핸들을 성공적으로 클릭한 상태에서 드래그 처리
    // -------------------------------------------------------------
    if (is_active && is_dragging_handle) {
        ImVec2 current_mouse_pos = ImGui::GetIO().MousePos;

        // 상대적 이동량 (Delta)
        ImVec2 offset = ImVec2(current_mouse_pos.x - drag_start_mouse_pos.x,
                               current_mouse_pos.y - drag_start_mouse_pos.y);

        // XY 이동 제한 (move_radius 범위)
        float len_sq = offset.x * offset.x + offset.y * offset.y;
        if (len_sq > move_radius * move_radius && len_sq > 0.0f) {
            float len = sqrtf(len_sq);
            offset.x = (offset.x / len) * move_radius;
            offset.y = (offset.y / len) * move_radius;
        }

        handle_pos = ImVec2(center.x + offset.x, center.y + offset.y);

        // 출력 좌표 계산 (-1.0f ~ 1.0f)
        output_x = offset.x / move_radius;
        output_y = -offset.y / move_radius; // Y축 상하 반전
    }

    // 포인터 인자 출력 연결
    if (out_x) *out_x = output_x;
    if (out_y) *out_y = output_y;

    // -------------------------------------------------------------
    // 그래픽 렌더링
    // -------------------------------------------------------------
    // 1. 배경 패드 (4각형)
    draw_list->AddRectFilled(p, ImVec2(p.x + pad_size.x, p.y + pad_size.y), IM_COL32(35, 35, 38, 255), 10.0f);
    draw_list->AddRect(p, ImVec2(p.x + pad_size.x, p.y + pad_size.y), IM_COL32(70, 70, 80, 255), 10.0f, 0, 1.5f);

    // 2. XY 이동 가이드 영역 (큰 원)
    draw_list->AddCircleFilled(center, move_radius, IM_COL32(45, 45, 50, 255), 64);
    draw_list->AddCircle(center, move_radius, IM_COL32(80, 80, 90, 255), 64, 1.0f);

    // 3. 십자 가이드선
    draw_list->AddLine(ImVec2(center.x - 8.0f, center.y), ImVec2(center.x + 8.0f, center.y), IM_COL32(100, 100, 100, 255));
    draw_list->AddLine(ImVec2(center.x, center.y - 8.0f), ImVec2(center.x, center.y + 8.0f), IM_COL32(100, 100, 100, 255));

    // 4. 중앙 작은 원 (조이스틱 핸들)
    ImU32 handle_color = is_dragging_handle ? IM_COL32(66, 150, 250, 255) : IM_COL32(200, 200, 200, 255);
    draw_list->AddCircleFilled(handle_pos, handle_radius, handle_color);
    draw_list->AddCircle(handle_pos, handle_radius, IM_COL32(255, 255, 255, 255), 0, 1.5f);

    // -------------------------------------------------------------
    // 패드 내부 하단 중앙에 가로 텍스트 정렬
    // -------------------------------------------------------------
    char text_buf[64];
    snprintf(text_buf, sizeof(text_buf), "X: %.2f  |  Y: %.2f", output_x, output_y);

    ImVec2 text_size = ImGui::CalcTextSize(text_buf);

    // 패드 하단 중앙 위치 계산 (패드 바닥에서 10px 위)
    float text_x = center.x - (text_size.x * 0.5f);
    float text_y = p.y + pad_size.y - text_size.y - 5.0f;
    ImVec2 text_pos = ImVec2(text_x, text_y);

    // 반투명 패널 (가독성 보장)
    draw_list->AddRectFilled(
        ImVec2(text_pos.x - 6.0f, text_pos.y - 2.0f),
        ImVec2(text_pos.x + text_size.x + 6.0f, text_pos.y + text_size.y + 2.0f),
        IM_COL32(20, 20, 22, 100),
        4.0f
    );

    // 텍스트 출력
    draw_list->AddText(text_pos, IM_COL32(220, 220, 220, 255), text_buf);
}



int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../example/imgui.ini");


    std::shared_ptr<Texture2D> texture1;
    std::shared_ptr<Texture2D> texture2;
    std::shared_ptr<Texture2D> texture3;




    if (!texture1)
        texture1 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO1.png");
    if (!texture2)
        texture2 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO2.png");
    if (!texture3)
        texture3 = ImGui::load_texture(IMGUI_ROOT "/data/RAKOKO3.png");


    while (ImGui::context([&]()
    {
        ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

        // 조이스틱
        static float joy_x = 0.0f;
        static float joy_y = 0.0f;
        joystic_controller(&joy_x, &joy_y);
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



    texture1.reset();
    texture2.reset();
    texture3.reset();
    ImGui::destroy();

    return 0;
}


