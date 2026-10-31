#pragma once
#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
    void* read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);
    void  read_line(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);    // ini 파일에서 우리가 정의한 섹션의 각 줄을 읽어올 때 호출
    void  write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);             // 프로그램 종료 시 또는 ini 파일 저장 시 호출
}
