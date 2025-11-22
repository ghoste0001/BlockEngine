#pragma once

#include "Gui.h"
#include "../datatypes/Task.h"

class TextEditor : public Gui {
public:
    int GetLineCount();
    void OnLog(const std::string& message);

    void Draw();

    void SetLuaState(lua_State* s) { L = s; }

    void ExecuteCurrentScript();
    void StopCurrentScript();

protected:
    std::string text;
    std::string title = "BlockEngine Script Editor";

    std::vector<std::string> history;

    lua_State* L = nullptr;
    bool executedScript = false;

    LuaTask* runningTask = nullptr; 
};
