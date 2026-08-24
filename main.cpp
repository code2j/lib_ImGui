#include "ui.hpp"
#include <iostream>


Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();





int main() {
    ImGui::init("테스트 프로그램", 1280, 720);
    ImGui::load_config("../imgui.ini");

    {
        // 스코프 안에서 생성하면 자동으로 해제됨
        ImGui::Texture texture1 = ImGui::load_texture("/home/jusik/workspace/lib_ImGui/data/RAKOKO1.png");

        while (!ImGui::should_close())
        {
        ImGui::context([&]()
        {
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
                ImGui::Check("체크박스 (커스텀)", &is_c);
                ImGui::Dummy(ImVec2(0, 15.0f));


                static int radio_idx = 0;
                ImGui::Radio("라디오 1 (커스텀)", &radio_idx, 0);
                ImGui::Radio("라디오 2 (커스텀)", &radio_idx, 1);
                ImGui::Dummy(ImVec2(0, 15.0f));

                if (ImGui::Button("일반 버튼", ImVec2(100, 0))) {
                    // 버튼 클릭 이벤트
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                if (ImGui::Button("위험 버튼", ImVec2(100, 0))) {
                    // 경고/삭제 버튼 이벤트
                }
                ImGui::PopStyleColor(3);
                ImGui::Dummy(ImVec2(0, 15.0f));


                // ==========================================
                // 2. 텍스트 입력 (Text Inputs)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_EDIT " Text Inputs");
                ImGui::Separator();

                static char text_buf[128] = "Hello, Custom ImGui!";
                ImGui::InputText("텍스트 입력", text_buf, IM_COUNTOF(text_buf));

                static char pw_buf[64] = "";
                ImGui::InputText("비밀번호", pw_buf, IM_COUNTOF(pw_buf), ImGuiInputTextFlags_Password);

                static char multiline_buf[256] = "여기에\n여러 줄의\n텍스트를 입력하세요.";
                ImGui::InputTextMultiline("메모", multiline_buf, IM_COUNTOF(multiline_buf), ImVec2(0, 60));
                ImGui::Dummy(ImVec2(0, 15.0f));



                // ==========================================
                // 3. 슬라이더 및 드래그 (Sliders & Drags)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TUNE " Sliders & Drags");
                ImGui::Separator();

                static double drag_value = 50.0f;
                ImGui::Drag("드래그 (커스텀)", &drag_value, 1.0f, 0.0f, 100.0f, "%.1f");

                static float slider_value = 30.0f;
                ImGui::SliderFloatX("슬라이더 FloatX", &slider_value, 0.0f, 100.0f, "%.1f");
                ImGui::SliderX("슬라이더 X", &slider_value, 0.0f, 100.0f, "%.1f");
                ImGui::Slider("슬라이더 (값 표시 숨김)", &slider_value, 0.0f, 100.0f);

                static float range_min = 20.0f, range_max = 80.0f;
                ImGui::SliderRangeX("범위 슬라이더 X", &range_min, &range_max, 0.0f, 100.0f);
                ImGui::SliderRange("범위 슬라이더 (기본)", &range_min, &range_max, 0.0f, 100.0f);
                ImGui::Dummy(ImVec2(0, 15.0f));

                // ==========================================
                // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
                ImGui::Separator();

                static int current_theme = 1;
                ImGui::DropDown("테마 선택 (DropDown)", &current_theme, "Light Theme\0Dark Theme\0Classic Theme\0", 3);

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


            if (ImGui::BeginCollapsingHeader(ICON_MD_CHAT_INFO " Notification Example ")) {
                // [알림 버튼 샘플]
                if (ImGui::Button(" 알림 정보 ")) {
                    ImGui::NotifyInfo("정보 알림이 표시 됩니다.");
                    std::cout << "[Info ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 성공 ")) {
                    ImGui::NotifySucc("성공 알림이 표시 됩니다.");
                    std::cout << "[Succ ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 경고 ")) {
                    ImGui::NotifyWarn("경고 알림이 표시 됩니다.");
                    std::cout << "[Warn ] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }
                ImGui::SameLine();

                if (ImGui::Button(" 알림 에러 ")) {
                    ImGui::NotifyError("에러 알림이 표시 됩니다.");
                    std::cout << "[Error] cout으로 출력된 문자열은 Imgui::Logger에 표시됩니다." << std::endl;
                }

                ImGui::EndCollapsingHeader();
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

                static float sliderx = 0.314f;
                ImGui::SliderX("SlideXr", &sliderx, 0.0f, 1.0f);
                ImGui::Dummy(ImVec2(0, 1));

                // [범위 슬라이더 샘플]
                static float price_min = 0.0f;
                static float price_max = 10.0f;
                ImGui::SliderRange("Range", &price_min, &price_max, 0.0f, 10.0f, "%.1f");
                ImGui::Dummy(ImVec2(0, 1));

                static int cnt = 1.0;
                if (ImGui::InputInt("Input", &cnt)) {
                    cnt = std::max(static_cast<int>(price_min), std::min(cnt, static_cast<int>(price_max)));
                }
                ImGui::Dummy(ImVec2(0, 1));

                ImGui::PopItemWidth();
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
                }
                ImGui::EndChild();
                ImGui::EndCollapsingHeader();

            }

            if (ImGui::BeginCollapsingHeader(ICON_MD_COLORS " Theme Example ")) {

                static int current_theme = 1;

                ImGui::BeginChild("ThemeSelector", ImVec2(0, 150), true);
                if (ImGui::ThemeSelector(&ImGuiExt::theme_id))
                {
                    // 테마가 변경되었을 때 실행할 로직
                    if (ImGuiExt::theme_id == 0) {
                        ImGui::style_white();
                    }
                    else {
                        ImGui::style_dark();
                    }

                }
                ImGui::EndChild();

                ImGui::EndCollapsingHeader();
            }

            ImGui::End();
            ImGui::ShowDemoWindow();

            ImGui::Begin(" " ICON_MD_GAMEPAD " TF 컨트롤 ");
            ImGui::TransformControl(&tf_control);
            ImGui::End();
        });
        }
    }


    ImGui::destroy();
    return 0;
}


