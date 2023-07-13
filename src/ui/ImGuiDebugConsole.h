#pragma once

#include <imgui.h>

// this struct is pretty much exactly ImGui's ExampleAppLog
namespace ImGuiUI {
    class DebugConsole
    {
        ImGuiTextBuffer     Buf;
        ImGuiTextFilter     Filter;
        ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
        ImVector<ImU32>     LineColors;  // List of line colors (changes depending on log severity)
        bool                AutoScroll;  // Keep scrolling if already at the bottom.
        bool                Timestamped; // Adds the current system time to the log.

        void AddLog(const char* fmt, va_list args, ImU32 col = IM_COL32_WHITE);

        void AddLogTimeStamped(const char* fmt, va_list args, ImU32 col = IM_COL32_WHITE);

    public:

        DebugConsole();

        void Clear();

        void LogWarn(const char *fmt, ...);

        void LogError(const char *fmt, ...);

        void LogInfo(const char *fmt, ...);

        void Draw(const char* title, bool* p_open = nullptr);
    };
}
