#pragma once
#include <windows.h>
#include <string>
#include "Settings.h"

enum class ConsoleColor {
    Default = 7,
    Blue = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    Green = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Red = FOREGROUND_RED | FOREGROUND_INTENSITY,
    Yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    Purple = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};

class ConsoleUI {
public:
    ConsoleUI();
    ~ConsoleUI();

    void setTextColor(ConsoleColor color);
    void printColored(const std::string& text, ConsoleColor color);
    void writeAt(COORD pos, const std::string& text, bool clearLine = true);
    void updateSettingsDisplay(int selectedOption, const SimulationSettings& settings);
    void updateStatusDisplay(bool running);
    void updateCS2StatusDisplay(bool detected);
    void updateDebugDisplay(const std::string& debugMsg, bool temporary = true);
    void updateInputDebugDisplay(const std::string& inputMsg);
    void updateErrorDisplay(const std::string& errorMsg, bool temporary = true);
    void clearTemporaryMessages();
    void drawSeparatorLine(COORD pos);
    void displayInstructions();
    int detectKeyPress();

    COORD getSettingsPos() const { return settingsStartPos_; }
    COORD getStatusPos() const { return statusPos_; }
    COORD getDebugPos() const { return debugPos_; }

private:
    HANDLE hConsole_;
    WORD originalAttributes_;
    DWORD originalConsoleMode_;
    CONSOLE_CURSOR_INFO originalCursorInfo_;
    COORD settingsStartPos_;
    COORD statusPos_;
    COORD debugPos_;
    COORD inputDebugPos_;
    COORD errorPos_; // New position for error messages
    std::string lastDebugMsg_;
    std::string lastInputMsg_;
    std::string lastErrorMsg_;
    std::chrono::steady_clock::time_point debugMsgTime_;
    std::chrono::steady_clock::time_point errorMsgTime_;
    bool debugMsgActive_;
    bool errorMsgActive_;
};
