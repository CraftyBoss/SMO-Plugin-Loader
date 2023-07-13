#include "ImGuiDebugConsole.h"
#include <time/seadDateTime.h>

namespace ImGuiUI
{
    DebugConsole::DebugConsole()
    {
        AutoScroll = true;
        Timestamped = true;
        Clear();
        LineColors.push_back(IM_COL32_WHITE); // add line color to list
    }

    void DebugConsole::Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);

        LineColors.clear();
    }

    void DebugConsole::AddLog(const char* fmt, va_list args, ImU32 col)
    {
        int old_size = Buf.size(); // get previous buffer size
        Buf.appendfv(fmt, args); // adds log message to text buffer
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1); // once we get to the end of the new string, push the index to the list of offsets

        LineColors.push_back(col); // add line color to list

    }

    void DebugConsole::AddLogTimeStamped(const char* fmt, va_list args, ImU32 col) {

        sead::DateTime curTime(0);
        curTime.setNow();
        sead::CalendarTime calTime;
        curTime.getCalendarTime(&calTime);

        char newFmt[18+strlen(fmt)];
        sprintf(newFmt, "[%02u:%02u:%02u] %s", calTime.getHour(), calTime.getMinute(), calTime.getSecond(), fmt);

        AddLog(newFmt, args, col);

    }

    void DebugConsole::LogInfo(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char newFmt[8+strlen(fmt)];
        sprintf(newFmt, "[Info] %s", fmt);

        ImU32 col = IM_COL32(200, 200, 200, 255);

        if(Timestamped)
            AddLogTimeStamped(newFmt, args, col);
        else
            AddLog(newFmt, args, col);

        va_end(args);
    }

    void DebugConsole::LogError(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char newFmt[8+strlen(fmt)];
        sprintf(newFmt, "[Error] %s", fmt);

        ImU32 col = IM_COL32(230, 20, 20, 255);

        if(Timestamped)
            AddLogTimeStamped(newFmt, args, col);
        else
            AddLog(newFmt, args, col);

        va_end(args);
    }

    void DebugConsole::LogWarn(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char newFmt[8+strlen(fmt)];
        sprintf(newFmt, "[Warn] %s", fmt);

        ImU32 col = IM_COL32(220, 230, 0, 255);

        if(Timestamped)
            AddLogTimeStamped(newFmt, args, col);
        else
            AddLog(newFmt, args, col);

        va_end(args);
    }

    void DebugConsole::Draw(const char* title, bool* p_open)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::Checkbox("Timestamp Logs", &Timestamped);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        // Filter settings
        const char* filterLabels[] = { "None", "Info", "Warn", "Error" };
        const char* filterValues[] = { "", "[Info]", "[Warn]", "[Error]" };
        static int item_current_idx = 0; // Here we store our selection data as an index.
        const char* curFilterLabel = filterLabels[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
        const char* curFilter = filterValues[item_current_idx];

        if (ImGui::BeginCombo("Filter Type", curFilterLabel))
        {
            for (int n = 0; n < IM_ARRAYSIZE(filterLabels); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(filterLabels[n], is_selected))
                    item_current_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if(strcmp(Filter.InputBuf, curFilter) != 0) {
            strncpy(Filter.InputBuf, curFilter, IM_ARRAYSIZE(Filter.InputBuf));
            Filter.Build();
        }

        ImGui::Separator();

        if (ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (clear)
                Clear();
            if (copy)
                ImGui::LogToClipboard();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            const char* buf = Buf.begin();
            const char* buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end)) {
                        if(LineColors.Size > line_no) {
                            ImGui::PushStyleColor(ImGuiCol_Text, LineColors[line_no]);
                            ImGui::TextUnformatted(line_start, line_end);
                            ImGui::PopStyleColor();
                        }else {
                            ImGui::TextUnformatted(line_start, line_end);
                        }
                    }
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   ImGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
                // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
                // within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
                // on your side is recommended. Using ImGuiListClipper requires
                // - A) random access into your data
                // - B) items all being the  same height,
                // both of which we can handle since we have an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display
                // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
                // it possible (and would be recommended if you want to search through tens of thousands of entries).
                ImGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {

                        const char* line_start = buf + LineOffsets[line_no];
                        const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;

                        if(LineColors.Size > line_no) {
                            ImGui::PushStyleColor(ImGuiCol_Text, LineColors[line_no]);
                            ImGui::TextUnformatted(line_start, line_end);
                            ImGui::PopStyleColor();
                        }else {
                            ImGui::TextUnformatted(line_start, line_end);
                        }

                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::End();
    }

}