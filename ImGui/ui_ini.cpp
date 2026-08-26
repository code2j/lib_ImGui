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

        if      (sscanf(line, "ShowLogWindow=%d", &val) == 1)   ImGui::show_log_window = (val != 0);
        else if (sscanf(line, "Show3DViewport=%d", &val) == 1)  ImGui::show_3d_viewport = (val != 0);
        else if (sscanf(line, "ShowSystemHud=%d", &val) == 1)   ImGui::show_system_hud = (val != 0);
        // else if (sscanf(line, "ShowMainMenu=%d", &val) == 1)    ImGui::show_menu = (val != 0);
        else if (sscanf(line, "Theme=%d", &val) == 1)           ImGui::theme_id = val;
    }


    void write_all(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
    {
        buf->appendf("[%s][Main]\n", handler->TypeName);
        buf->appendf("ShowLogWindow=%d\n", ImGui::show_log_window ? 1 : 0);
        buf->appendf("Show3DViewport=%d\n", ImGui::show_3d_viewport ? 1 : 0);
        buf->appendf("ShowSystemHud=%d\n", ImGui::show_system_hud ? 1 : 0);
        // buf->appendf("ShowMainMenu=%d\n", ImGui::show_menu ? 1 : 0);
        buf->appendf("Theme=%d\n", ImGui::theme_id);
        buf->appendf("\n");
    }
}
