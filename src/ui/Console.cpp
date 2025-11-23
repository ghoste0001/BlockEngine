#include "Console.h"
#include <raylib.h>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <stdexcept>

#include "../../dependencies/luau/VM/include/lua.h"

extern lua_State* L_main;

ImVec4 textColorNormal = ImVec4(1, 1, 1, 1);
ImVec4 textColorCommand = ImVec4(0.6, 0.6, 1, 1);
ImVec4 textColorWarning = ImVec4(1, 1, 0.2, 1);
ImVec4 textColorError = ImVec4(1, 0.4, 0.4, 1);

// https://stackoverflow.com/questions/15006269/c-get-substring-before-a-certain-char
std::string subStrBeforeChar(std::string s, std::string c) {
    std::string::size_type pos = s.find(c);
    if (pos != std::string::npos) return s.substr(0, pos);
    else return s;
}

std::string subStrAfterChar(std::string s, std::string c) {
    std::string::size_type pos = s.find(c);
    if (pos != std::string::npos) return s.substr(pos + 1, s.length());
    else return "";
}

// https://cplusplus.com/forum/beginner/251052/
std::string trim(const std::string& line) {
    const char* WhiteSpace = " \t\v\r\n";
    std::size_t start = line.find_first_not_of(WhiteSpace);
    std::size_t end = line.find_last_not_of(WhiteSpace);
    return start == end ? std::string() : line.substr(start, end - start + 1);
}

std::string GetTimestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm_buf;

    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_buf, &t);
    #else
        localtime_r(&t, &tm_buf);
    #endif

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "[%02d:%02d:%02d.%03d]", 
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count());

    return std::string(buffer);
}

void Console::ClearLog() {
    history.clear();
    colors.clear();
}

void Console::Reset() {
    ClearLog();
    memset(inputBuf, 0, sizeof(inputBuf));
    historyPos = -1;
}

void Console::Log(std::string text, ImVec4 color) {
    // add max history size (cause it was starting to lag after a while)
    if (history.size() >= 2000) {
        history.erase(history.begin());
        colors.erase(colors.begin());
    }
    history.push_back(GetTimestamp()+" "+text);
    colors.push_back(color);
}

void Console::Warn(std::string text) {
    Console::Log(text, textColorWarning);
}

void Console::Error(std::string text) {
    Console::Log(text, textColorError);
}

void Console::ExecCommand(std::string text) {
    std::string cmdLog = "$ ";
    cmdLog.append(text);
    Console::Log(cmdLog, textColorCommand);

    std::string cmd = trim(subStrBeforeChar(text, " "));

    if (cmd == "help") {
        Console::Log("Commands:");
        Console::Log("- help: show this message");
        Console::Log("- max_fps <int>: set the maximum fps");
        Console::Log("- controls: controls for the camera");
        Console::Log("- clear: clear console output");
        Console::Log("- luatasks: get number of tasks running");
        Console::Log("- anything else: execute text as lua script");
    } else if (cmd == "clear") {
        Console::ClearLog();
    } else if (cmd == "controls") {
        Console::Log("Controls:");
        Console::Log("- WASD: move forward/left/backwards/rightwards");
        Console::Log("- RightClick, Arrow Keys: rotate camera");
        Console::Log("- Space, E: go upward");
        Console::Log("- Q: go downward");
        Console::Log("- Scroll: move toward/away from cursor");
    } else if (cmd == "luatasks") {
        Console::Log(std::to_string(g_tasks.size()));
        } else if (cmd == "max_fps") {
        size_t firstSpace = text.find(' ');
        size_t pos = (firstSpace == std::string::npos) ? std::string::npos : text.find_first_not_of(" \t", firstSpace + 1);
        std::string scriptText = (pos == std::string::npos) ? std::string() : text.substr(pos);
        try {
            int fps = std::stoi(scriptText);
            if (fps < 10) {
            Console::Error("FPS must be at least 10");
            return;
            }
            Console::Log("Changed max fps to " + std::to_string(fps));
            SetTargetFPS(fps);
        } catch (const std::invalid_argument&) {
            Console::Error("Invalid fps value: not a number");
        } catch (const std::out_of_range&) {
            Console::Error("Invalid fps value: out of range");
        }
    } else {
        if (!text.empty()) {
            if (!Task_TryRun(L_main, text)) {
                Console::Error("Failed to execute lua script");
            }
        }
    }
}

void Console::OnLog(const std::string& message) {
    Console::Log(message);
}

static int TextEditCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        // up/down arrow

    }

    return 0;
}

void Console::Draw() {
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(520, 430), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_Once);

    if (!ImGui::Begin(title.c_str(), &visible)) {
        ImGui::End();
        return;
    }

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) Console::ClearLog();

            ImGui::EndPopup();
        }

        for (size_t i = 0; i < history.size(); i++) {
            ImGui::TextColored(colors[i], "%s", history[i].c_str());
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
    }

    ImGui::Separator();

    // bool reclaim_focus = false; -- unused
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", inputBuf, IM_ARRAYSIZE(inputBuf), input_text_flags, &TextEditCallback)) {
        std::string input = inputBuf;
        if (!input.empty()) {
            Console::ExecCommand(inputBuf);

            historyPos = -1;
        }

        inputBuf[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }


    ImGui::End();
}
