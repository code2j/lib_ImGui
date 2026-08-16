#include "ui.hpp"
#include <iostream>

#define FILE_PCD IMGUI_ROOT "/aaa.ply"

Eigen::Matrix4d tf_control = Eigen::Matrix4d::Identity();



namespace CustomImGui {

    bool BeginAnimatedCollapsingHeader(const char* label, bool default_open = false) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        ImGuiID id = window->GetID(label);

        bool is_open = window->StateStorage.GetInt(id, default_open ? 1 : 0);
        float anim_t = window->StateStorage.GetFloat(id + 1, is_open ? 1.0f : 0.0f);
        float max_height = window->StateStorage.GetFloat(id + 2, 0.0f);

        bool calculating_height = (is_open && max_height == 0.0f);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight());
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

        ImGui::ItemSize(size);
        if (!ImGui::ItemAdd(bb, id)) {
            return false;
        }

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed) {
            is_open = !is_open;
            window->StateStorage.SetInt(id, is_open ? 1 : 0);
        }

        ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImGui::GetStyle().FrameRounding);

        ImVec2 arrow_pos(bb.Min.x + ImGui::GetStyle().FramePadding.x, bb.Min.y + ImGui::GetStyle().FramePadding.y);
        ImGui::RenderArrow(window->DrawList, arrow_pos, ImGui::GetColorU32(ImGuiCol_Text), is_open ? ImGuiDir_Down : ImGuiDir_Right);

        ImVec2 text_pos(arrow_pos.x + g.FontSize + ImGui::GetStyle().FramePadding.x * 2.0f, arrow_pos.y);
        ImGui::RenderText(text_pos, label);

        if (!calculating_height) {
            float speed = ImGui::GetIO().DeltaTime * 7.0f;
            anim_t = ImClamp(anim_t + (is_open ? speed : -speed), 0.0f, 1.0f);
            window->StateStorage.SetFloat(id + 1, anim_t);
        }

        if (anim_t > 0.0f || calculating_height) {
            float current_height = calculating_height ? 0.0f : (max_height * anim_t);
            float alpha = calculating_height ? 0.0f : anim_t;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
            ImGuiChildFlags child_flags = 0;

            if (calculating_height || (is_open && anim_t >= 1.0f)) {
                child_flags |= ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY;
                current_height = 0.0f;
            }

            ImGui::BeginChild(id + 3, ImVec2(0, current_height), child_flags, window_flags);
            return true;
        }

        return false;
    }

    void EndAnimatedCollapsingHeader(const char* label) {
        // 1. Child 윈도우 내부에서 그려진 내용물의 높이를 먼저 계산합니다.
        float height = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;

        // 2. Child 윈도우를 닫고 적용했던 스타일을 해제합니다.
        ImGui::EndChild();
        ImGui::PopStyleVar(3);

        // 3. [핵심 수정] EndChild() 이후에 부모 윈도우를 가져와야 정상적으로 상태를 저장할 수 있습니다.
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGuiID id = window->GetID(label);

        bool is_open = window->StateStorage.GetInt(id, 0);
        float anim_t = window->StateStorage.GetFloat(id + 1, 0.0f);
        float max_height = window->StateStorage.GetFloat(id + 2, 0.0f);

        // 4. 최초 측정 시(max_height == 0) 이거나 완전히 열려있을 때 높이를 갱신합니다.
        if (is_open && (max_height == 0.0f || anim_t >= 1.0f)) {
            window->StateStorage.SetFloat(id + 2, height);
        }
    }

} // namespace CustomImGui


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
                "시스템 시작",
                "시스템 초기화",
                "시스템 준비",
                "시스템 동작",
            };

            static int current_account_status = 2;

            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0);
            ImGui::BeginChild("상태바", ImVec2(0, 150), true);
            if (ImGui::InputInt("상태", &current_account_status)) {
                current_account_status = std::max(0, std::min(current_account_status, 3));
            }
            ImGui::StatusStepBar("##AccountStatusStepBar", &current_account_status, status_labels, 4);
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }


        if (ImGui::CollapsingHeader(ICON_MD_IMAGE " 이미지 ")) {
            // 토클 이미지 보이기
           static bool show_image = false;
           if (ImGui::ToggleButton("이미지 보이기", &show_image)) {
               ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Info, "이미지가 표시됩니다."));
               std::cout << "cout으로 출력된 메세지는 로그에 나타납니다." << std::endl;
           }
           ImGui::SameLine();
           ImGui::Text("이미지 보이기");

            // ImGui::Checkbox()



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
        }


        static float volume = 0.5f;
        static bool is_fullscreen = false;

        if (CustomImGui::BeginAnimatedCollapsingHeader("고급 설정 (Animated)", false)) {

            ImGui::Text("이곳에 부드럽게 펼쳐질 내용물들을 넣습니다.");
            ImGui::SliderFloat("볼륨", &volume, 0.0f, 1.0f);
            ImGui::Checkbox("전체 화면", &is_fullscreen);

            if (ImGui::Button("적용하기")) {
                // 동작
            }


            CustomImGui::EndAnimatedCollapsingHeader("고급 설정 (Animated)");
        }

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


