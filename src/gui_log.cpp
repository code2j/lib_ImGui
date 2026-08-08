#include "gui_log.h"
#include "icon.h"

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

void ImGuiLogWindow::draw(const char *title, bool *p_open)
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title)) {
        ImGui::End();
        return;
    }

    // ---------------------------------------------------------------
    // 1. 로그 지우기 버튼 (+ 툴팁)
    // ---------------------------------------------------------------
    if (ImGui::Button(ICON_MD_DELETE)) clear();

    // 버튼 위에 마우스가 있을 때 툴팁 표시
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("로그 지우기");
    }

    ImGui::SameLine();


    // ---------------------------------------------------------------
    // 2. 오토 스크롤 토글 버튼 (+ 툴팁)
    // ---------------------------------------------------------------
    if (auto_scroll) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    }

    if (ImGui::Button(ICON_MD_VERTICAL_ALIGN_BOTTOM)) {
        auto_scroll = !auto_scroll;
    }

    ImGui::PopStyleColor(2);

    if (ImGui::IsItemHovered()) {
        if (auto_scroll) {
            ImGui::SetTooltip("자동 스크롤 (현재: 켜짐)");
        } else {
            ImGui::SetTooltip("자동 스크롤 (현재: 꺼짐)");
        }
    }

    ImGui::Separator();

    // 스크롤 가능한 로그 텍스트 영역
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    // 텍스트 렌더링
    ImGui::TextUnformatted(buf.begin(), buf.end());

    // 자동 스크롤 로직
    if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
