#include "ui.hpp"


namespace ImGui
{
void* read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
    return (void*)1;
}


void read_line(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line)
{
    // ini 파일에서 우리가 정의한 섹션의 각 줄을 읽어올 때 호출
    int val;

    if      (sscanf(line, "ShowLogWindow=%d", &val) == 1)   ImGuiExt::show_log_window = (val != 0);
    else if (sscanf(line, "Show3DViewport=%d", &val) == 1)  ImGuiExt::show_3d_viewport = (val != 0);
}


void write_all(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
    // 프로그램 종료 시 또는 ini 파일 저장 시 호출
    buf->appendf("[%s][Main]\n", handler->TypeName);
    buf->appendf("ShowLogWindow=%d\n", ImGuiExt::show_log_window ? 1 : 0);
    buf->appendf("Show3DViewport=%d\n", ImGuiExt::show_3d_viewport ? 1 : 0);
    buf->appendf("\n");
}
} // namespace ImGui
