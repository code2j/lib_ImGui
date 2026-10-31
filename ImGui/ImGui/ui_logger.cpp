#include "ui_logger.h"
#include "ui_font_icon.h"
#include "ui.hpp"
#include <iostream>

namespace
{
    // std::cout 연결에 필요한 streambuf 구현은 외부 API에서 숨긴다.
    class LoggerStream : public std::streambuf
    {
    public:
        bool auto_scroll = true;
        ImGuiTextBuffer buf;
        std::streambuf* old_buf = nullptr;

        ~LoggerStream() override { Restore(); }

        void Restore()
        {
            if (old_buf != nullptr)
            {
                if (std::cout.rdbuf() == this)
                    std::cout.rdbuf(old_buf);
                old_buf = nullptr;
            }
        }

    protected:
        int_type overflow(int_type c) override
        {
            if (traits_type::eq_int_type(c, traits_type::eof()))
                return traits_type::not_eof(c);

            const char ch = traits_type::to_char_type(c);
            buf.append(&ch, &ch + 1);
            if (old_buf != nullptr)
                return old_buf->sputc(ch);
            return c;
        }

        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            if (n <= 0)
                return 0;
            buf.append(s, s + n);
            return old_buf != nullptr ? old_buf->sputn(s, n) : n;
        }

        int sync() override
        {
            return old_buf != nullptr ? old_buf->pubsync() : 0;
        }
    };

    LoggerStream& GetLogger()
    {
        static LoggerStream logger;
        return logger;
    }
}

void ImGui::InitLogger()
{
    LoggerStream& logger = GetLogger();
    if (logger.old_buf != nullptr)
        return;
    logger.buf.clear();
    logger.auto_scroll = true;
    logger.old_buf = std::cout.rdbuf(&logger);
}

void ImGui::ShutdownLogger()
{
    GetLogger().Restore();
}

void ImGui::ClearLogger()
{
    GetLogger().buf.clear();
}

void ImGui::ShowLogger(const char *title, bool open)
{
    if (!open)
        return;

    LoggerStream& logger = GetLogger();
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
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(125, 125, 125, 20));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(125, 125, 125, 30));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    // 로그 지우기 버튼
    if (ImGui::IconButton(ICON_MD_DELETE, BUTTON_SIZE, ImGuiButtonFlags_None)) {
        ClearLogger();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("모두 삭제");
    }


    // 오토 스크롤 토글 버튼
    if (logger.auto_scroll) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    }

    if (ImGui::IconButton(ICON_MD_WRAP_TEXT, BUTTON_SIZE, ImGuiButtonFlags_None)) {
        logger.auto_scroll = !logger.auto_scroll;
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

    ImGui::PushFont(ImGui::LOGGER_FONT);
    ImGui::Indent(10.0f);
    ImGui::TextUnformatted(logger.buf.begin(), logger.buf.end()); // 텍스트 렌더링
    ImGui::Unindent(10.0f);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    // 자동 스크롤 로직
    if (logger.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}
