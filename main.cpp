#include "ui.hpp"
#include <iostream>

#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();


int main() {
    ImGui::Init("테스트 프로그램", 1280, 720);
    ImGui::LoadConfig("../imgui.ini");

    bool is_loading = true;
    ImGui::Texture texture1 = ImGui::load_texture(IMGUI_ROOT "/image.png");


    while (!ImGui::ShouldClose()) {
    ImGui::Context([&]()
    {


        // ----------------------
        // 제어 패널
        // ----------------------
        ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

        if (ImGui::BeginCollapsingHeader(ICON_MD_WIDGETS " Widgets Example ")) {
            ImGui::BeginChild("Widgets_Child", ImVec2(0, 1150), true);

            // ==========================================
            // 1. 기본 컨트롤 (Basic Controls)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_CHECK_BOX " Basic Controls");
            ImGui::Separator();

            static bool is_c = false;
            if (ImGui::Check("체크박스", &is_c)) {
                if (is_c)
                    ImGui::NotifyInfo("체크박스 선택됨");
                else
                    ImGui::NotifyInfo("체크박스 해제됨");
            }

            static int radio_idx = 0;
            if (ImGui::Radio("라디오 1", &radio_idx, 0))
                ImGui::NotifyInfo("라디오 1 선택됨");
            if (ImGui::Radio("라디오 2", &radio_idx, 1))
                ImGui::NotifyInfo("라디오 2 선택됨");

            static bool is_t = false;
            if (ImGui::Toggle("토글", &is_t)) {
                if (is_t)
                    ImGui::NotifyInfo("토글 선택됨");
                else
                    ImGui::NotifyInfo("토글 해제됨");
            }

            // 버튼
            float btn_width = ImGui::GetContentRegionAvail().x / 4.15f;
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(100, 141, 222, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(120, 161, 242, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 80, 121, 202, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n정보", ImVec2(btn_width, 0))) {
                ImGui::NotifyInfo("정보 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32( 46, 160,  67, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32( 66, 180,  87, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 26, 140,  47, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n성공", ImVec2(btn_width, 0))) {
                ImGui::NotifySucc("성공 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(227, 179,  65, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(247, 199,  85, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(207, 159,  45, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n경고", ImVec2(btn_width, 0))) {
                ImGui::NotifyWarn("경고 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(224, 108, 117, 255)); // 기본
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(244, 128, 137, 255)); // 호버 (밝게)
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(204,  88,  97, 255)); // 클릭 (어둡게)
            if (ImGui::Button("알림\n에러", ImVec2(btn_width, 0))) {
                ImGui::NotifyError("에러 메세지 표시");
            }
            ImGui::PopStyleColor(3);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 2. 텍스트 입력 (Text Inputs)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_EDIT " Text Inputs");
            ImGui::Separator();

            static char text_buf[128] = "Hello, Custom ImGui!";
            ImGui::InputTextLine("텍스트 입력", text_buf, IM_COUNTOF(text_buf));

            static char pw_buf[64] = "";
            ImGui::InputTextLine("비밀번호", pw_buf, IM_COUNTOF(pw_buf), ImGuiInputTextFlags_Password);

            static char multiline_buf[256] = "여기에\n여러 줄의\n텍스트를 입력하세요.";
            ImGui::InputTextLines("메모", multiline_buf, IM_COUNTOF(multiline_buf), ImVec2(0, 80));
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 3. 슬라이더 및 드래그 (Sliders & Drags)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TUNE " Sliders & Drags");
            ImGui::Separator();

            static double drag_value = 50.0f;
            ImGui::Drag("Drag", &drag_value, 1.0f, 0.0f, 100.0f, "%.1f");

            static float slider_value = 30.0f;
            ImGui::Slider("Slider2", &slider_value, 0.0f, 100.0f);

            static float range_min = 20.0f, range_max = 80.0f;
            ImGui::SliderRange("Slider Range2", &range_min, &range_max, 0.0f, 100.0f);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // Input Number
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_CONFIRMATION_NUMBER " Input Number");
            ImGui::Separator();

            static int input_scalar_value = 50;
            ImGui::InputNum("Input Scalar", &input_scalar_value);

            static double input_double_value = 50.0;
            ImGui::InputNum("Input Double", &input_double_value, 0.01, 0, "%0.2f");
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
            ImGui::Separator();

            static int current_theme = 1;
            ImGui::DropDown("DropDown", &current_theme, "선택 1\0선택 2\0선택 3\0", 3);

            static int listbox_item_current = 1;
            const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange" };
            ImGui::List("과일 목록", &listbox_item_current, listbox_items, IM_COUNTOF(listbox_items), 4);
            ImGui::Dummy(ImVec2(0, 20.0f));


            // ==========================================
            // 5. 색상 선택기 (Color Pickers)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_COLOR_LENS " Color Pickers");
            ImGui::Separator();

            static float color_rgb[3] = { 0.36f, 0.41f, 0.94f };
            ImGui::ColorEdit3X("컬러3", color_rgb);

            static float color_rgba[4] = { 0.8f, 0.2f, 0.3f, 0.5f };
            ImGui::ColorEdit4X("컬러4", color_rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);

            ImGui::EndChild();
            ImGui::EndCollapsingHeader();
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_JOYSTICK " Joystick Example ")) {
            // [조이스틱 샘플]
            ImVec2 joy;
            ImGui::Joystic(&joy);
            ImGui::Dummy(ImVec2(0, 20));
            ImGui::EndCollapsingHeader();
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
            ImGui::BeginChild("상태바", ImVec2(0, 230), true);

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
            if (ImGui::InputNum("##상태", &current_account_status)) {
                current_account_status = std::max(0, std::min(current_account_status, 3));
            }
            ImGui::PopItemWidth();

            ImGui::EndChild();
            ImGui::PopStyleVar();

            ImGui::EndCollapsingHeader();
        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_IMAGE " Image Example ")) {
            // 토클 이미지 보이기
            static bool show_image = false;
            if (ImGui::Toggle("이미지 보이기", &show_image)) {
                ImGui::NotifyInfo("이미지가 표시됩니다.");
            }
            ImGui::SameLine();
            ImGui::Text("이미지 보이기");


            ImGui::BeginChild("child", ImVec2(0, 400), true);
            if (show_image) {
                ImVec2 avail = ImGui::GetContentRegionAvail();

                if (auto tex = texture1.lock()) {
                    float img_aspect = (float) tex->width / (float) tex->height;
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

                    ImGui::Image(tex->id, size);
                }

            }
            ImGui::EndChild();
            ImGui::EndCollapsingHeader();

        }

        if (ImGui::BeginCollapsingHeader(ICON_MD_COLORS " Theme Example ")) {

            static int current_theme = 1;

            ImGui::BeginChild("ThemeSelector", ImVec2(0, 150), true);
            if (ImGui::ThemeSelector(&ImGui::theme_id))
            {
                // 테마가 변경되었을 때 실행할 로직
                if (ImGui::theme_id == 0) {
                    ImGui::theme_white();
                }
                else {
                    ImGui::theme_dark();
                }

            }
            ImGui::EndChild();

            ImGui::EndCollapsingHeader();
        }

        ImGui::End();
    });  }



    ImGui::Destroy();
    return 0;
}


