#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();


// 테마 미리보기를 위한 색상 데이터 구조체
struct ThemePreviewData
{
    const char* Name;
    ImU32 BgColor;
    ImU32 PanelColor;
    ImU32 PrimaryColor;
    ImU32 SecondaryColor;
    ImU32 TextColor;
};

// current_theme: 0 = Light, 1 = Dark (외부에서 상태를 관리하기 위해 포인터로 받음)
bool RenderThemeSelector(int* current_theme)
{
    bool value_changed = false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // 두 가지 테마 정의 (Light / Dark)
    ThemePreviewData themes[2] = {
        {
            "Light Mode",
            IM_COL32(240, 243, 249, 255), // Bg: 밝은 회청색
            IM_COL32(255, 255, 255, 255), // Panel: 순백색
            IM_COL32(59, 104, 255, 255),  // Primary: 블루
            IM_COL32(255, 99, 132, 255),  // Secondary: 핑크/레드
            IM_COL32(50, 50, 55, 255)     // Text: 어두운 회색
        },
        {
            "Dark Mode",
            IM_COL32(25, 25, 28, 255),    // Bg: 어두운 배경
            IM_COL32(36, 36, 40, 255),    // Panel: 약간 더 밝은 패널
            IM_COL32(93, 137, 255, 255),  // Primary: 밝은 블루
            IM_COL32(187, 134, 252, 255), // Secondary: 퍼플
            IM_COL32(230, 230, 230, 255)  // Text: 밝은 회색
        }
    };

    // 카드 크기 설정
    const ImVec2 card_size(140.0f, 130.0f);
    const float preview_h = 90.0f; // 상단 미리보기 영역 높이
    const float rounding = 8.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16.0f, 10.0f));

    for (int i = 0; i < 2; i++)
    {
        if (i > 0) ImGui::SameLine();

        ImGui::PushID(i);

        const ThemePreviewData& theme = themes[i];
        bool is_selected = (*current_theme == i);

        // 카드의 Bounding Box 계산
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + card_size.x, pos.y + card_size.y));

        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, window->GetID("##ThemeCard")))
        {
            ImGui::PopID();
            continue;
        }

        // 클릭 이벤트 처리
        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, window->GetID("##ThemeCard"), &hovered, &held);
        if (pressed)
        {
            *current_theme = i;
            value_changed = true;
        }

        // ==========================================
        // 1. 카드 배경 및 테두리 (Hover & Selection)
        // ==========================================
        ImU32 card_bg = ImGui::GetColorU32(ImGuiCol_WindowBg); // 현재 ImGui 테마의 윈도우 배경색 사용
        ImU32 border_col = is_selected ? theme.PrimaryColor :
                           (hovered ? IM_COL32(150, 150, 150, 100) : IM_COL32(100, 100, 100, 50));

        window->DrawList->AddRectFilled(bb.Min, bb.Max, card_bg, rounding);
        window->DrawList->AddRect(bb.Min, bb.Max, border_col, rounding, 0, is_selected ? 2.0f : 1.0f);

        // ==========================================
        // 2. 미니 UI 미리보기 (상단)
        // ==========================================
        ImVec2 p_min = ImVec2(bb.Min.x + 2.0f, bb.Min.y + 2.0f);
        ImVec2 p_max = ImVec2(bb.Max.x - 2.0f, bb.Min.y + preview_h);

        // 미리보기 전체 배경
        window->DrawList->AddRectFilled(p_min, p_max, theme.BgColor, rounding - 2.0f, ImDrawFlags_RoundCornersTop);

        // 가짜 UI: 상단 헤더 (Header)
        ImVec2 header_p_max = ImVec2(p_max.x, p_min.y + 16.0f);
        window->DrawList->AddRectFilled(p_min, header_p_max, theme.PanelColor, rounding - 2.0f, ImDrawFlags_RoundCornersTop);
        // 헤더 안의 가짜 텍스트 (타이틀)
        window->DrawList->AddRectFilled(ImVec2(p_min.x + 8.0f, p_min.y + 6.0f), ImVec2(p_min.x + 40.0f, p_min.y + 10.0f), theme.TextColor);

        // 가짜 UI: 좌측 사이드바 (Sidebar)
        ImVec2 sidebar_p_max = ImVec2(p_min.x + 30.0f, p_max.y);
        window->DrawList->AddRectFilled(ImVec2(p_min.x, header_p_max.y + 4.0f), sidebar_p_max, theme.PanelColor, 0.0f);
        // 사이드바 안의 가짜 메뉴 아이템들
        for (int j = 0; j < 3; j++) {
            float sy = header_p_max.y + 12.0f + (j * 12.0f);
            window->DrawList->AddRectFilled(ImVec2(p_min.x + 6.0f, sy), ImVec2(p_min.x + 24.0f, sy + 4.0f), theme.TextColor);
        }

        // 가짜 UI: 메인 컨텐츠 영역 패널
        ImVec2 content_min = ImVec2(p_min.x + 38.0f, header_p_max.y + 12.0f);
        ImVec2 content_max = ImVec2(p_max.x - 8.0f, p_max.y - 8.0f);
        window->DrawList->AddRectFilled(content_min, content_max, theme.PanelColor, 4.0f);

        // 가짜 UI: Primary Button
        ImVec2 btn1_min = ImVec2(content_min.x + 8.0f, content_min.y + 8.0f);
        ImVec2 btn1_max = ImVec2(content_min.x + 40.0f, content_min.y + 20.0f);
        window->DrawList->AddRectFilled(btn1_min, btn1_max, theme.PrimaryColor, 3.0f);

        // 가짜 UI: Secondary Button / Badge
        ImVec2 btn2_min = ImVec2(btn1_max.x + 6.0f, content_min.y + 8.0f);
        ImVec2 btn2_max = ImVec2(btn2_min.x + 18.0f, content_min.y + 20.0f);
        window->DrawList->AddRectFilled(btn2_min, btn2_max, theme.SecondaryColor, 3.0f);

        // 가짜 UI: 본문 텍스트 줄
        for (int j = 0; j < 2; j++) {
            float ty = btn1_max.y + 10.0f + (j * 8.0f);
            float width = (j == 0) ? 35.0f : 20.0f; // 두 번째 줄은 짧게
            window->DrawList->AddRectFilled(ImVec2(content_min.x + 8.0f, ty), ImVec2(content_min.x + 8.0f + width, ty + 3.0f), theme.TextColor);
        }

        // 미리보기와 아래 텍스트 사이의 구분선
        window->DrawList->AddLine(ImVec2(bb.Min.x, p_max.y), ImVec2(bb.Max.x, p_max.y), IM_COL32(100, 100, 100, 50));

        // ==========================================
        // 3. 하단 테마 이름 텍스트
        // ==========================================
        ImVec2 text_size = ImGui::CalcTextSize(theme.Name);
        ImVec2 text_pos = ImVec2(
            bb.Min.x + (card_size.x - text_size.x) * 0.5f,
            bb.Min.y + preview_h + ((card_size.y - preview_h) - text_size.y) * 0.5f
        );

        // 선택된 상태면 Primary 색상으로, 아니면 기본 텍스트 색상으로 출력
        ImU32 name_col = is_selected ? theme.PrimaryColor : ImGui::GetColorU32(ImGuiCol_Text);
        window->DrawList->AddText(text_pos, name_col, theme.Name);

        ImGui::PopID();
    }

    ImGui::PopStyleVar();

    return value_changed;
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
            ImGui::Joystic(&joy);
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

        if (ImGui::BeginCollapsingHeader(ICON_MD_COLORS " Theme Example ")) {

            static int current_theme = 1;

            ImGui::BeginChild("ThemeSelector", ImVec2(0, 150), true);
            if (RenderThemeSelector(&current_theme))
            {
                // 테마가 변경되었을 때 실행할 로직
                if (current_theme == 0) {
                    // ImGui::StyleColorsLight();
                    // 또는 커스텀 Light 색상 적용 로직
                } else {
                    // ImGui::StyleColorsDark();
                    // 또는 커스텀 Dark 색상 적용 로직
                }
            }
            ImGui::EndChild();

            ImGui::EndCollapsingHeader(ICON_MD_COLORS " Theme Example ");
        }

        ImGui::End();



        ImGui::ShowDemoWindow();


        ImGui::Begin(" " ICON_MD_GAMEPAD " TF 컨트롤 ");
        ImGui::TransformControl(&tf_control);
        ImGui::End();

        points.move(tf_control);
        ImGui::draw_points(points);
    }));

    }


    ImGui::destroy();
    return 0;
}


