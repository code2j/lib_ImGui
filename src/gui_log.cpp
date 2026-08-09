#include "gui_log.h"
#include "icon.h"
#include "font2.cpp"


int ImGuiLogWindow::overflow(int c)
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

std::streamsize ImGuiLogWindow::xsputn(const char *s, std::streamsize n) {
    buf.append(s, s + n);

    if (old_buf) {
        old_buf->sputn(s, n);
    }
    return n;
}

ImGuiLogWindow::ImGuiLogWindow()
{
    auto_scroll = true;
    old_buf = std::cout.rdbuf(this);
}

ImGuiLogWindow::~ImGuiLogWindow()
{
    std::cout.rdbuf(old_buf);
}

void ImGuiLogWindow::clear()
{
    buf.clear();
}

void ImGuiLogWindow::load_font()
{
    const float font_size = 17.0;

    D2Cording = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
        font2_compressed_data,
        font2_compressed_size,
        font_size,
        NULL,
        ImGui::GetIO().Fonts->GetGlyphRangesKorean()
    );
}

void ImGuiLogWindow::draw(const char *title, bool *p_open)
{
    const ImVec2 BUTTON_SIZE = ImVec2(35, 35);

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title)) {
        ImGui::End();
        return;
    }

    // 왼쪽 사이드바 너비 지정 (버튼 크기 + 약간의 여백)
    float sidebar_width = BUTTON_SIZE.x + 0.0f;

    // ===============================================================
    // 1. 왼쪽 패널: 아이콘 버튼들 (세로 배치)
    // ===============================================================
    ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, 0), false);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
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

    ImGui::PopStyleColor(); // 텍스트 색상 Pop

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("자동 스크롤");
    }

    // 스타일 복구
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::EndChild(); // 왼쪽 패널 종료


    ImGui::SameLine();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    ImGui::PushFont(D2Cording);
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
