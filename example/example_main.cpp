#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();


// 테마 미리보기를 위한 색상 데이터 구조체


// current_theme: 0 = Light, 1 = Dark (외부에서 상태를 관리하기 위해 포인터로 받음)




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
            if (ImGui::ThemeSelector(&ImGuiExt::theme_id))
            {
                // 테마가 변경되었을 때 실행할 로직
                if (ImGuiExt::theme_id == 0) {
                    auto& colors = ImGui::GetStyle().Colors;

                    ImVec4 color_bg                 = ImColor(251, 251, 251); // v
                    ImVec4 color_surf               = ImColor(235, 235, 237);
                    ImVec4 color_surf_variant       = ImColor(246, 246, 246);

                    ImVec4 color_primary            = ImColor(93, 105, 240);
                    ImVec4 color_primary_hover      = ImColor(73, 85, 185);
                    ImVec4 color_primary_active     = ImColor(63, 74, 162);


                    ImVec4 color_red                = ImColor(181, 65, 60);
                    ImVec4 color_green              = ImColor(87, 242, 135);

                    ImVec4 color_text               = ImColor(40, 40, 45);      // v
                    ImVec4 color_text_disabled      = ImColor(128, 133, 138);
                    ImVec4 color_transparent        = ImColor(0, 0, 0, 0);


                    ImVec4 color_tab_hovered        = ImColor(200, 200, 200);
                    ImVec4 color_tab_focused        = ImColor(235, 235, 237); // v
                    ImVec4 color_tab_active         = ImColor(235, 235, 237);

                    ImVec4 color_border             = ImColor(213, 213, 217); // v

                    ImVec4 color_scrollbar          = ImColor(92, 93, 103);
                    ImVec4 color_scrollbar_hover    = ImColor(71, 77, 82);
                    ImVec4 color_scrollbar_active   = ImColor(82, 87, 92);


                    // ---------------------------------------------------------------
                    // ImGui 색상 적용
                    // ---------------------------------------------------------------
                    // [Text]
                    colors[ImGuiCol_Text]                  = color_text;
                    colors[ImGuiCol_TextDisabled]          = color_text_disabled;
                    colors[ImGuiCol_TextSelectedBg]        = color_primary;
                    colors[ImGuiCol_DragDropTarget]        = color_primary_active;
                    // [Background]
                    colors[ImGuiCol_WindowBg]              = color_bg;
                    colors[ImGuiCol_ChildBg]               = color_surf;
                    colors[ImGuiCol_PopupBg]               = color_surf;
                    colors[ImGuiCol_MenuBarBg]             = color_surf;
                    // [Border]
                    colors[ImGuiCol_Border]                = color_border;
                    colors[ImGuiCol_BorderShadow]          = color_transparent;
                    // [Frame]
                    colors[ImGuiCol_FrameBg]               = color_surf_variant;
                    colors[ImGuiCol_FrameBgHovered]        = color_transparent;
                    colors[ImGuiCol_FrameBgActive]         = color_transparent;
                    // [Title]
                    colors[ImGuiCol_TitleBg]               = color_bg;
                    colors[ImGuiCol_TitleBgActive]         = color_bg;
                    colors[ImGuiCol_TitleBgCollapsed]      = color_bg;
                    // [Scrollbar]
                    colors[ImGuiCol_ScrollbarBg]           = color_transparent;
                    colors[ImGuiCol_ScrollbarGrab]         = color_scrollbar;
                    colors[ImGuiCol_ScrollbarGrabHovered]  = color_scrollbar_hover;
                    colors[ImGuiCol_ScrollbarGrabActive]   = color_scrollbar_active;
                    // [Checkbox]
                    colors[ImGuiCol_CheckMark]             = color_primary;
                    // [Slider]
                    colors[ImGuiCol_SliderGrab]            = color_primary;
                    colors[ImGuiCol_SliderGrabActive]      = color_primary_hover;
                    // [Button]
                    colors[ImGuiCol_Button]                = color_primary;
                    colors[ImGuiCol_ButtonHovered]         = color_primary_hover;
                    colors[ImGuiCol_ButtonActive]          = color_primary_active;
                    // [Header]
                    colors[ImGuiCol_Header]                = color_tab_focused;
                    colors[ImGuiCol_HeaderHovered]         = color_tab_hovered;
                    colors[ImGuiCol_HeaderActive]          = color_tab_active;
                    // [Separator]
                    colors[ImGuiCol_Separator]             = ImVec4(0.28, 0.29, 0.30, 1.00);
                    colors[ImGuiCol_SeparatorHovered]      = color_primary_hover;
                    colors[ImGuiCol_SeparatorActive]       = color_primary_active;
                    // [Resize Grip]
                    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 1.00);
                    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00);
                    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00);
                    // [Tab]
                    colors[ImGuiCol_Tab]                   = color_bg;
                    colors[ImGuiCol_TabHovered]            = color_primary_hover;
                    colors[ImGuiCol_TabSelected]           = color_tab_active;
                    colors[ImGuiCol_TabUnfocused]          = color_bg;
                    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.24, 0.34, 0.44, 1.00);
                    colors[ImGuiCol_TabSelectedOverline]   = color_transparent;
                    colors[ImGuiCol_TabDimmed]             = color_bg;
                    colors[ImGuiCol_TabDimmedSelected]     = color_tab_active;
                    // [Plot]
                    colors[ImGuiCol_PlotLines]             = color_primary;
                    colors[ImGuiCol_PlotLinesHovered]      = color_primary_hover;
                    colors[ImGuiCol_PlotHistogram]         = color_primary;
                    colors[ImGuiCol_PlotHistogramHovered]  = color_primary_hover;
                    // [Table]
                    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20, 0.22, 0.24, 1.00);
                    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.28, 0.29, 0.30, 1.00);
                    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.24, 0.25, 0.26, 1.00);
                    colors[ImGuiCol_TableRowBg]            = ImVec4(0.20, 0.22, 0.24, 1.00);
                    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.22, 0.24, 0.26, 1.00);
                    // [Nav]
                    colors[ImGuiCol_NavCursor];
                    colors[ImGuiCol_NavHighlight]          = ImVec4(0.46, 0.56, 0.66, 1.00);
                    colors[ImGuiCol_NavWindowingHighlight] = color_green;
                    colors[ImGuiCol_NavWindowingDimBg]     = color_red;
                    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80, 0.80, 0.80, 0.35);
                    // [Docking]
                    colors[ImGuiCol_DockingPreview]        = color_primary_active;
                    colors[ImGuiCol_DockingEmptyBg]        = color_primary_hover;
                }
                else {
                    auto& colors = ImGui::GetStyle().Colors;

                    ImVec4 color_bg                 = ImColor(7, 7, 9);
                    ImVec4 color_surf               = ImColor(12, 12, 14);
                    ImVec4 color_surf_variant       = ImColor(11, 11, 12);


                    ImVec4 color_primary            = ImColor(93, 105, 240);
                    ImVec4 color_primary_hover      = ImColor(73, 85, 185);
                    ImVec4 color_primary_active     = ImColor(63, 74, 162);


                    ImVec4 color_red                = ImColor(181, 65, 60);
                    ImVec4 color_green              = ImColor(87, 242, 135);

                    ImVec4 color_text               = ImColor(228, 228, 230);
                    ImVec4 color_text_disabled      = ImColor(128, 133, 138);
                    ImVec4 color_transparent        = ImColor(0, 0, 0, 0);


                    ImVec4 theme_tab_focused        = ImColor(18, 18, 20);
                    ImVec4 theme_tab_active         = ImColor(36, 36, 39);

                    ImVec4 color_border             = ImColor(44, 44, 47);

                    ImVec4 color_scrollbar          = ImColor(92, 93, 103);
                    ImVec4 color_scrollbar_hover    = ImColor(71, 77, 82);
                    ImVec4 color_scrollbar_active   = ImColor(82, 87, 92);


                    // ---------------------------------------------------------------
                    // ImGui 색상 적용
                    // ---------------------------------------------------------------
                    // [Text]
                    colors[ImGuiCol_Text]                  = color_text;
                    colors[ImGuiCol_TextDisabled]          = color_text_disabled;
                    colors[ImGuiCol_TextSelectedBg]        = color_primary;
                    colors[ImGuiCol_DragDropTarget]        = color_primary_active;
                    // [Background]
                    colors[ImGuiCol_WindowBg]              = color_bg;
                    colors[ImGuiCol_ChildBg]               = color_surf;
                    colors[ImGuiCol_PopupBg]               = color_surf;
                    colors[ImGuiCol_MenuBarBg]             = color_surf;
                    // [Border]
                    colors[ImGuiCol_Border]                = color_border;
                    colors[ImGuiCol_BorderShadow]          = color_transparent;
                    // [Frame]
                    colors[ImGuiCol_FrameBg]               = color_surf_variant;
                    colors[ImGuiCol_FrameBgHovered]        = color_transparent;
                    colors[ImGuiCol_FrameBgActive]         = color_transparent;
                    // [Title]
                    colors[ImGuiCol_TitleBg]               = color_bg;
                    colors[ImGuiCol_TitleBgActive]         = color_bg;
                    colors[ImGuiCol_TitleBgCollapsed]      = color_bg;
                    // [Scrollbar]
                    colors[ImGuiCol_ScrollbarBg]           = color_transparent;
                    colors[ImGuiCol_ScrollbarGrab]         = color_scrollbar;
                    colors[ImGuiCol_ScrollbarGrabHovered]  = color_scrollbar_hover;
                    colors[ImGuiCol_ScrollbarGrabActive]   = color_scrollbar_active;
                    // [Checkbox]
                    colors[ImGuiCol_CheckMark]             = color_primary;
                    // [Slider]
                    colors[ImGuiCol_SliderGrab]            = color_primary;
                    colors[ImGuiCol_SliderGrabActive]      = color_primary_hover;
                    // [Button]
                    colors[ImGuiCol_Button]                = color_primary;
                    colors[ImGuiCol_ButtonHovered]         = color_primary_hover;
                    colors[ImGuiCol_ButtonActive]          = color_primary_active;
                    // [Header]
                    colors[ImGuiCol_Header]                = theme_tab_focused;
                    colors[ImGuiCol_HeaderHovered]         = color_primary_hover;
                    colors[ImGuiCol_HeaderActive]          = color_primary_active;
                    // [Separator]
                    colors[ImGuiCol_Separator]             = ImVec4(0.28, 0.29, 0.30, 1.00);
                    colors[ImGuiCol_SeparatorHovered]      = color_primary_hover;
                    colors[ImGuiCol_SeparatorActive]       = color_primary_active;
                    // [Resize Grip]
                    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36, 0.46, 0.56, 1.00);
                    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40, 0.50, 0.60, 1.00);
                    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.44, 0.54, 0.64, 1.00);
                    // [Tab]
                    colors[ImGuiCol_Tab]                   = color_bg;
                    colors[ImGuiCol_TabHovered]            = color_primary_hover;
                    colors[ImGuiCol_TabSelected]           = theme_tab_active;
                    colors[ImGuiCol_TabUnfocused]          = color_bg;
                    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.24, 0.34, 0.44, 1.00);
                    colors[ImGuiCol_TabSelectedOverline]   = color_transparent;
                    colors[ImGuiCol_TabDimmed]             = color_bg;
                    colors[ImGuiCol_TabDimmedSelected]     = theme_tab_active;
                    // [Plot]
                    colors[ImGuiCol_PlotLines]             = color_primary;
                    colors[ImGuiCol_PlotLinesHovered]      = color_primary_hover;
                    colors[ImGuiCol_PlotHistogram]         = color_primary;
                    colors[ImGuiCol_PlotHistogramHovered]  = color_primary_hover;
                    // [Table]
                    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20, 0.22, 0.24, 1.00);
                    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.28, 0.29, 0.30, 1.00);
                    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.24, 0.25, 0.26, 1.00);
                    colors[ImGuiCol_TableRowBg]            = ImVec4(0.20, 0.22, 0.24, 1.00);
                    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.22, 0.24, 0.26, 1.00);
                    // [Nav]
                    colors[ImGuiCol_NavCursor];
                    colors[ImGuiCol_NavHighlight]          = ImVec4(0.46, 0.56, 0.66, 1.00);
                    colors[ImGuiCol_NavWindowingHighlight] = color_green;
                    colors[ImGuiCol_NavWindowingDimBg]     = color_red;
                    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80, 0.80, 0.80, 0.35);
                    // [Docking]
                    colors[ImGuiCol_DockingPreview]        = color_primary_active;
                    colors[ImGuiCol_DockingEmptyBg]        = color_primary_hover;
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


