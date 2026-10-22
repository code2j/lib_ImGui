#include "ui.hpp"
#include <iostream>

#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();


int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../imgui.ini");
    ImGui::open_menu(true);

    bool is_loading = true;
    ImGui::Texture texture1 = ImGui::load_texture(IMGUI_ROOT "/image.png");


    while (!ImGui::should_close()) {
    ImGui::context([&]()
    {


        // ----------------------
        // 제어 패널
        // ----------------------
        ImGui::Begin(" " ICON_MD_TUNE " 제어 패널 ");

        if (ImGui::BeginCollapsingHeader(ICON_MD_WIDGETS " Widgets Example ")) {
            // Child 창 시작
            ImGui::BeginChild("Widgets_Child", ImVec2(0, 1300), true);

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

            if (ImGui::Button("알림 - 정보", ImVec2(100, 0))) {
                ImGui::NotifyInfo("정보 메세지 표시");
            }
            ImGui::SameLine();

            if (ImGui::Button("알림 - 성공", ImVec2(100, 0))) {
                ImGui::NotifySucc("성공 메세지 표시");
            }
            ImGui::SameLine();

            if (ImGui::Button("알림 - 경고", ImVec2(100, 0))) {
                ImGui::NotifyWarn("경고 메세지 표시");
            }
            ImGui::SameLine();

            if (ImGui::Button("알림 - 에러", ImVec2(100, 0))) {
                ImGui::NotifyError("에러 메세지 표시");
            }
            ImGui::SameLine();

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
            // ImGui::SliderX("Slider1", &slider_value, 0.0f, 100.0f, "%.1f");
            ImGui::Slider("Slider2", &slider_value, 0.0f, 100.0f);

            static float range_min = 20.0f, range_max = 80.0f;
            // ImGui::SliderRangeX("Slider Range1", &range_min, &range_max, 0.0f, 100.0f);
            ImGui::SliderRange("Slider Range2", &range_min, &range_max, 0.0f, 100.0f);
            ImGui::Dummy(ImVec2(0, 15.0f));


            // ==========================================
            // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
            ImGui::Separator();

            static int current_theme = 1;
            ImGui::DropDown("DropDown", &current_theme, "선택 1\0선택 2\0선택 3\0", 3);

            static int listbox_item_current = 1;
            const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange" };
            ImGui::ListBox("과일 목록", &listbox_item_current, listbox_items, IM_COUNTOF(listbox_items), 4);
            ImGui::Dummy(ImVec2(0, 15.0f));


            // ==========================================
            // 5. 색상 선택기 (Color Pickers)
            // ==========================================
            ImGui::Dummy(ImVec2(0, 15.0f));
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_COLOR_LENS " Color Pickers");
            ImGui::Separator();

            static float color_rgb[3] = { 0.36f, 0.41f, 0.94f };
            ImGui::ColorEdit3("테마 색상", color_rgb);

            static float color_rgba[4] = { 0.8f, 0.2f, 0.3f, 0.5f };
            ImGui::ColorEdit4("알파 포함 색상", color_rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
            ImGui::Dummy(ImVec2(0, 15.0f));


            // ==========================================
            // 6. 상태 및 진행률 (Misc / Progress)
            // ==========================================
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_INFO " Status & Misc");
            ImGui::Separator();

            static float progress = 0.0f;
            progress += 0.005f; // 애니메이션 효과를 위해 임의 증가
            if (progress > 1.0f) progress = 0.0f;

            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "로딩 중...");

            ImGui::TextWrapped("위젯 위로 마우스를 올리면 툴팁을 확인할 수 있습니다.");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("이것은 ImGui 표준 툴팁입니다.");
            }

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

        // ImGui::ShowDemoWindow();





    });  }



    ImGui::destroy();
    return 0;
}


