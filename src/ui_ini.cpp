#include "ui.hpp"


namespace ImGui
{
    void* read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
    {
        return (void*)1;
    }


    void read_line(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line)
    {
        int val;

        if      (sscanf(line, "ShowLogWindow=%d", &val) == 1)   ImGuiExt::show_log_window = (val != 0);
        else if (sscanf(line, "Show3DViewport=%d", &val) == 1)  ImGuiExt::show_3d_viewport = (val != 0);
    }


    void write_all(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
    {
        buf->appendf("[%s][Main]\n", handler->TypeName);
        buf->appendf("ShowLogWindow=%d\n", ImGuiExt::show_log_window ? 1 : 0);
        buf->appendf("Show3DViewport=%d\n", ImGuiExt::show_3d_viewport ? 1 : 0);
        buf->appendf("\n");
    }
} // namespace ImGui
