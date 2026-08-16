#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();

namespace ImGui
{
    // 커스텀 상태 진행바 위젯
    // 사용 예시:
    // const char* labels[] = { "양호해요!", "제한되었어요", "극도로 제한되\n었어요", "위험해요", "정지됐어요" };
    // static int current_status = 0;
    // ImGui::StatusStepBar("##AccountStatus", &current_status, labels, 5);
    bool StatusStepBar(const char* str_id, int* current_step, const char** step_labels, int num_steps)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(str_id);

        // 위젯 크기 및 배치 계산
        float width = CalcItemWidth();
        if (width <= 0.0f) width = GetContentRegionAvail().x;

        float circle_radius = 12.0f; // 원의 반지름
        float line_thickness = 4.0f; // 연결 선의 두께
        float line_gap = 6.0f;       // 원과 선 사이의 간격

        // 텍스트를 위한 대략적인 높이 계산 (2줄까지 지원)
        float text_height = g.FontSize * 2.5f;
        float total_height = circle_radius * 2.0f + 10.0f + text_height;

        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, total_height));
        ItemSize(frame_bb, style.FramePadding.y);
        if (!ItemAdd(frame_bb, id))
            return false;

        // 양끝 여백을 확보하여 원이 잘리지 않도록 간격 계산
        float usable_width = width - (circle_radius * 2.0f) - 40.0f; // 좌우 20px씩 여백
        float start_x = frame_bb.Min.x + 20.0f + circle_radius;
        float start_y = frame_bb.Min.y + circle_radius + 5.0f;
        float step_spacing = usable_width / (float)(num_steps - 1);

        bool value_changed = false;
        ImDrawList* draw_list = window->DrawList;

        // 색상 정의 (이미지 기반)
        ImU32 active_circle_col = IM_COL32(98, 192, 115, 255);  // 부드러운 그린
        ImU32 inactive_circle_col = IM_COL32(43, 45, 49, 255);  // 다크 그레이 (원)
        ImU32 line_col = IM_COL32(43, 45, 49, 255);             // 다크 그레이 (선)
        ImU32 text_col = IM_COL32(230, 230, 230, 255);          // 밝은 텍스트
        ImU32 icon_col = IM_COL32(20, 20, 20, 255);             // 체크 아이콘 (검은색에 가까움)

        // 1. 배경 연결 선 그리기
        for (int i = 0; i < num_steps - 1; ++i)
        {
            ImVec2 p1 = ImVec2(start_x + i * step_spacing + circle_radius + line_gap, start_y);
            ImVec2 p2 = ImVec2(start_x + (i + 1) * step_spacing - circle_radius - line_gap, start_y);
            draw_list->AddLine(p1, p2, line_col, line_thickness);
        }

        // 2. 원, 아이콘, 텍스트 렌더링 및 상호작용
        for (int i = 0; i < num_steps; ++i)
        {
            ImVec2 center = ImVec2(start_x + i * step_spacing, start_y);

            // 각 원(Step)의 클릭 영역
            ImRect circle_bb(
                ImVec2(center.x - circle_radius - 10.0f, center.y - circle_radius - 10.0f),
                ImVec2(center.x + circle_radius + 10.0f, center.y + circle_radius + 10.0f)
            );

            bool hovered, held;
            ImGuiID step_id = window->GetID((void*)(intptr_t)i);
            bool pressed = ButtonBehavior(circle_bb, step_id, &hovered, &held);

            if (pressed && *current_step != i)
            {
                *current_step = i;
                value_changed = true;
                MarkItemEdited(id);
            }

            // 호버 효과 (마우스 올렸을 때 살짝 밝게)
            ImU32 current_circle_col = (i == *current_step) ? active_circle_col : inactive_circle_col;
            if (hovered && i != *current_step)
            {
                current_circle_col = IM_COL32(65, 67, 74, 255); // 호버된 비활성 원 색상
            }

            // 원 그리기
            draw_list->AddCircleFilled(center, circle_radius, current_circle_col);

            // 현재 단계(활성화)인 경우 체크 아이콘 그리기
            if (i == *current_step)
            {
                // ICON_MD_CHECK (Material Design Icon 문자열 사용, 설정에 맞게 변경 가능)
                const char* icon = ICON_MD_CHECK;
                ImVec2 icon_size = CalcTextSize(icon);
                draw_list->AddText(ImVec2(center.x - icon_size.x * 0.5f, center.y - icon_size.y * 0.5f), icon_col, icon);
            }

            // 하단 텍스트 렌더링 (멀티 라인 중앙 정렬 지원)
            if (step_labels && step_labels[i])
            {
                const char* text = step_labels[i];
                ImVec2 text_pos = ImVec2(center.x, center.y + circle_radius + 12.0f);

                // \n 기준으로 문자열을 잘라서 각각 가운데 정렬
                const char* line_start = text;
                const char* line_end = strchr(line_start, '\n');
                while (line_start != NULL)
                {
                    if (!line_end) line_end = line_start + strlen(line_start);
                    ImVec2 line_size = CalcTextSize(line_start, line_end);

                    // 각 줄 중앙 정렬 출력
                    draw_list->AddText(ImVec2(center.x - line_size.x * 0.5f, text_pos.y), text_col, line_start, line_end);

                    text_pos.y += g.FontSize + 2.0f; // 다음 줄 Y 좌표
                    if (*line_end == '\0') break;

                    line_start = line_end + 1;
                    line_end = strchr(line_start, '\n');
                }
            }
        }

        return value_changed;
    }
}


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





        if (ImGui::CollapsingHeader(ICON_MD_CHAT_INFO " 알림 버튼 샘플 ")) {
            // [알림 버튼 샘플]
            if (ImGui::Button(" 알림 정보 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "정보 알림이 표시 됩니다."));
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 성공 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Success, "성공 알림이 표시 됩니다."));
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 경고 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Warning, "경고 알림이 표시 됩니다."));
            }
            ImGui::SameLine();

            if (ImGui::Button(" 알림 에러 ")) {
                ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, "에러 알림이 표시 됩니다."));
            }
        }


        if (ImGui::CollapsingHeader(ICON_MD_TUNE " 슬라이더 샘플 ")) {
            // [슬라이더 샘플]
            static float drag = 0.0f;
            static float slider = 0.0f;
            ImGui::DragFloat("드레그", &drag);
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::SliderFloat("슬라이더", &slider, 0.0f, 1.0f);
        }


        if (ImGui::CollapsingHeader(ICON_MD_JOYSTICK " 조이스틱 샘플 ")) {
            // [조이스틱 샘플]
            ImVec2 joy;
            ImGui::joystic(&joy);
            ImGui::Dummy(ImVec2(0, 20));
        }

        if (ImGui::CollapsingHeader(ICON_MD_STEPPERS " 상태 바 ")) {
            const char* status_labels[] = {
                "양호해요!",
                "제한되었어요",
                "극도로 제한되\n었어요", // 개행 처리 시 자동으로 두 줄 모두 가운데 정렬됨
                "위험해요",
                "정지됐어요"
            };

            static int current_account_status = 3;

            ImGui::StatusStepBar("##AccountStatusStepBar", &current_account_status, status_labels, 5);
        }






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
        ImGui::tf_widget(&tf_control);
        ImGui::End();

        points.move(tf_control);
        ImGui::draw_points(points);
    }));

    }


    ImGui::destroy();
    return 0;
}


