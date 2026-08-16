#include "ui_logger.h"
#include "ui_icon.h"
#include "ui.hpp"


namespace
{
    bool            auto_scroll;
    ImGuiTextBuffer buf;
    std::streambuf* old_buf;
}


int ImGuiLogger::overflow(int c)
{
    if (c != EOF) {
        // 1. ImGui 버퍼에 텍스트 저장
        buf.appendf("%c", (char)c);

        // 2. 기존 터미널(콘솔) 버퍼에도 동일하게 출력 (추가된 부분)
        if (old_buf) {
            old_buf->sputc(c);
        }
    }
    return c;
}

std::streamsize ImGuiLogger::xsputn(const char *s, std::streamsize n)
{
    buf.append(s, s + n);

    if (old_buf) {
        old_buf->sputn(s, n);
    }
    return n;
}

ImGuiLogger::ImGuiLogger()
{
    auto_scroll = true;
    old_buf = std::cout.rdbuf(this);
}

ImGuiLogger::~ImGuiLogger()
{
    std::cout.rdbuf(old_buf);
}

void ImGuiLogger::clear()
{
    buf.clear();
}

void ImGuiLogger::draw(const char *title, bool *p_open)
{
    const ImVec2 BUTTON_SIZE = ImVec2(35, 35);

    if (!ImGui::Begin(title)) {
        ImGui::End();
        return;
    }

    // 왼쪽 사이드바 너비 지정 (버튼 크기 + 약간의 여백)
    float sidebar_width = BUTTON_SIZE.x + 0.0f;

    // ===============================================================
    //  왼쪽 패널: 아이콘 버튼들 (세로 배치)
    // ===============================================================
    ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, 0), false);

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(39, 39, 43, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(34, 34, 37, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    // 로그 지우기 버튼
    if (ImGui::Button(ICON_MD_DELETE, BUTTON_SIZE)) {
        clear();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("모두 삭제");
    }


    // 오토 스크롤 토글 버튼
    if (auto_scroll) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    }

    if (ImGui::Button(ICON_MD_VERTICAL_ALIGN_BOTTOM, BUTTON_SIZE)) {
        auto_scroll = !auto_scroll;
    }

    ImGui::PopStyleColor(3); // 텍스트 색상 Pop

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("자동 스크롤");
    }

    // 스타일 복구
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::EndChild(); // 왼쪽 패널 종료
    ImGui::SameLine();


    // ===============================================================
    // 텍스트 출력 그리기
    // ===============================================================
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::PushFont(ImGuiExt::D2Cording);
    ImGui::Indent(10.0f);
    ImGui::TextUnformatted(buf.begin(), buf.end()); // 텍스트 렌더링
    ImGui::Unindent(10.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    // 자동 스크롤 로직
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
