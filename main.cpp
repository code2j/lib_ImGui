#include "ui.hpp"
#include "ImGuizmo.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <limits>
#include <cmath>

#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();




int main(int argc, char **argv) {



    // ImGui 초기화
    ImGui::Init("테스트 프로그램", 1280, 720);
    ImGui::LoadConfig("../imgui.ini");

    // 좌측 메뉴에 빈 도킹 영역을 추가한다.
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널0");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널1");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널2");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널3");
    ImGui::AddTab(ICON_MD_TUNE "  사용자 패널4");







    ImGui::Image2D texture1 = ImGui::LoadImage(IMGUI_ROOT "/image.png");


    while (!ImGui::ShouldClose()) {


    ImGui::Context([&]()
    {

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
            ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_NUMBERS " Input Number");
            ImGui::Separator();

            static int input_scalar_value = 50;
            ImGui::InputNum("Input Int", &input_scalar_value);

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

                if (texture1 && texture1->id != 0) {
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
                    ImGui::ThemeWhite();
                }
                else {
                    ImGui::ThemeDark();
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



// namespace ImGui {
// enum class HudInputType { Key, MouseLeft, MouseRight, MouseLeftHold, MouseRightHold };
// struct HudHint { const char* key; const char* description; HudInputType type; };
//
// void ControlsHud(const ImVec2& origin, const ImVec2& size, const HudHint* hints, int count)
// {
//     if (!hints || count <= 0 || size.x <= 100.0f || size.y <= 50.0f) return;
//     ImDrawList* draw = ImGui::GetWindowDrawList();
//     const float padding = 12.0f, line = ImGui::GetTextLineHeight();
//     const float row_height = line + 10.0f, header_height = 0.0f;
//     float key_width = 0.0f, description_width = 0.0f;
//     for (int i = 0; i < count; ++i) {
//         const auto& hint = hints[i];
//         key_width = std::max(key_width, ImGui::CalcTextSize(hint.key).x);
//         if (hint.type == HudInputType::MouseLeftHold || hint.type == HudInputType::MouseRightHold)
//             key_width = std::max(key_width, 18.0f + 2.0f * (2.0f + ImGui::CalcTextSize("꾸욲~").x * 0.7f));
//         description_width = std::max(description_width, ImGui::CalcTextSize(hint.description).x);
//     }
//     const float badge_width = key_width + 16.0f;
//     const float width = std::min(size.x - 16.0f, std::max(185.0f, padding * 3 + badge_width + description_width));
//     const float height = std::min(size.y - 16.0f, header_height + padding + row_height * count);
//     const ImVec2 pos(origin.x + 8.0f, origin.y + size.y - height - 8.0f);
//     const ImVec2 end(pos.x + width, pos.y + height);
//     draw->PushClipRect(pos, end, true);
//     draw->AddRectFilled(pos, end, ImGui::GetColorU32(ImGuiCol_ChildBg, 0.88f), 8.0f);
//     draw->AddRect(pos, end, ImGui::GetColorU32(ImGuiCol_Border), 8.0f);
//     for (int i = 0; i < count; ++i) {
//         const float y = pos.y + header_height + 6.0f + i * row_height;
//         const ImVec2 badge_min(pos.x + padding, y);
//         const ImVec2 badge_max(badge_min.x + badge_width, y + line + 4.0f);
//         if (hints[i].type != HudInputType::Key) {
//             const bool hold = hints[i].type == HudInputType::MouseLeftHold || hints[i].type == HudInputType::MouseRightHold;
//             const float mouse_width = 18.0f;
//             const float mx = badge_min.x + (badge_width - mouse_width) * 0.5f;
//             const float my = y, mouse_height = line + 4.0f;
//             draw->AddRectFilled(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height), ImGui::GetColorU32(ImGuiCol_FrameBg), 7.0f);
//             const bool left = hints[i].type == HudInputType::MouseLeft || hints[i].type == HudInputType::MouseLeftHold;
//             draw->AddRectFilled(ImVec2(mx + (left ? 1.0f : mouse_width * 0.5f), my + 1.0f),
//                 ImVec2(mx + (left ? mouse_width * 0.5f : mouse_width - 1.0f), my + mouse_height * 0.48f),
//                 ImGui::GetColorU32(ImGuiCol_Button), 6.0f, left ? ImDrawFlags_RoundCornersTopLeft : ImDrawFlags_RoundCornersTopRight);
//             draw->AddRect(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height), ImGui::GetColorU32(ImGuiCol_TextDisabled), 7.0f);
//             draw->AddLine(ImVec2(mx, my + mouse_height * 0.48f), ImVec2(mx + mouse_width, my + mouse_height * 0.48f), ImGui::GetColorU32(ImGuiCol_TextDisabled));
//             draw->AddLine(ImVec2(mx + mouse_width * 0.5f, my + 1.0f), ImVec2(mx + mouse_width * 0.5f, my + mouse_height * 0.48f), ImGui::GetColorU32(ImGuiCol_TextDisabled));
//             if (hold) {
//                 const float font_size = ImGui::GetFontSize() * 0.7f;
//                 draw->AddText(ImGui::GetFont(), font_size, ImVec2(mx + mouse_width + 2.0f, my), IM_COL32(255, 215, 80, 255), "꾸욲~");
//             }
//         } else {
//             const float rounding = 5.0f;
//             const float text_width = ImGui::CalcTextSize(hints[i].key).x;
//             const float cap_width = std::max(line + 4.0f, text_width + 16.0f);
//             const float cap_x = badge_min.x + (badge_width - cap_width) * 0.5f;
//             const ImVec2 cap_min(cap_x, badge_min.y);
//             const ImVec2 cap_max(cap_x + cap_width, badge_max.y);
//             const ImVec2 face_max(cap_max.x, cap_max.y - 3.0f);
//             draw->AddRectFilled(cap_min, cap_max, ImGui::GetColorU32(ImGuiCol_Border), rounding);
//             draw->AddRectFilled(cap_min, face_max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
//             draw->AddRect(cap_min, face_max, ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f), rounding);
//             draw->AddText(ImVec2(cap_x + (cap_width - text_width) * 0.5f, y + 0.5f), ImGui::GetColorU32(ImGuiCol_Text), hints[i].key);
//         }
//         draw->AddText(ImVec2(badge_max.x + padding, y + 2.0f), ImGui::GetColorU32(ImGuiCol_Text), hints[i].description);
//     }
//     draw->PopClipRect();
// }
// }