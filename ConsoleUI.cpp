#include "ConsoleUI.h"
#include <conio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "Utils.h"

ConsoleUI::ConsoleUI() : hConsole_(GetStdHandle(STD_OUTPUT_HANDLE)) {
    SMALL_RECT windowSize = { 0, 0, 119, 39 };
    SetConsoleWindowInfo(hConsole_, TRUE, &windowSize);

    COORD bufferSize = { 120, 40 };
    SetConsoleScreenBufferSize(hConsole_, bufferSize);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole_, &csbi);
    originalAttributes_ = csbi.wAttributes;

    GetConsoleMode(hConsole_, &originalConsoleMode_);
    SetConsoleMode(hConsole_, originalConsoleMode_ & ~ENABLE_QUICK_EDIT_MODE);

    // Hide cursor initially
    GetConsoleCursorInfo(hConsole_, &originalCursorInfo_);
    CONSOLE_CURSOR_INFO cursorInfo = originalCursorInfo_;
    cursorInfo.bVisible = FALSE; // Ensure cursor is invisible
    cursorInfo.dwSize = 100; // Size doesn't matter since it's invisible
    SetConsoleCursorInfo(hConsole_, &cursorInfo);
    SetConsoleTextAttribute(hConsole_, FOREGROUND_RED | FOREGROUND_INTENSITY);

    settingsStartPos_ = { 0, 9 };
    statusPos_ = { 0, static_cast<SHORT>(settingsStartPos_.Y + 9) };
    debugPos_ = { 0, static_cast<SHORT>(statusPos_.Y + 3) };
    inputDebugPos_ = { 0, static_cast<SHORT>(debugPos_.Y + 2) };
    errorPos_ = { 0, static_cast<SHORT>(inputDebugPos_.Y + 2) };

    debugMsgActive_ = false;
    errorMsgActive_ = false;
}

ConsoleUI::~ConsoleUI() {
    SetConsoleMode(hConsole_, originalConsoleMode_);
    SetConsoleCursorInfo(hConsole_, &originalCursorInfo_);
    SetConsoleTextAttribute(hConsole_, originalAttributes_);
}

void ConsoleUI::setTextColor(ConsoleColor color) {
    SetConsoleTextAttribute(hConsole_, static_cast<WORD>(color));
}

void ConsoleUI::printColored(const std::string& text, ConsoleColor color) {
    setTextColor(color);
    std::cout << text;
    setTextColor(ConsoleColor::Default);

    // Ensure cursor remains hidden after printing
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole_, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole_, &cursorInfo);
}

void ConsoleUI::writeAt(COORD pos, const std::string& text, bool clearLine) {
    SetConsoleCursorPosition(hConsole_, pos);
    if (clearLine) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole_, &csbi);
        DWORD written;
        FillConsoleOutputCharacter(hConsole_, ' ', csbi.dwSize.X - pos.X, pos, &written);
        SetConsoleCursorPosition(hConsole_, pos);
    }
    std::cout << text << std::flush;

    // Ensure cursor remains hidden after writing
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole_, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole_, &cursorInfo);
}

void ConsoleUI::updateSettingsDisplay(int selectedOption, const SimulationSettings& settings) {
    const char* options[] = { "auto-activate", "update rate (hz)", "m_yaw", "cl_yawspeed", "+left key", "+right key", "modifier key", "modifier", "auto-activate toggle key" };
    for (int i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        COORD pos = { 0, static_cast<SHORT>(settingsStartPos_.Y + i) };
        std::string prefix = (i == selectedOption) ? "> " : "  ";
        std::string value;

        if (i == 0) { // auto-activate
            std::string text = prefix + options[i] + ": ";
            writeAt(pos, text, true);
            SetConsoleCursorPosition(hConsole_, { static_cast<SHORT>(text.length()), pos.Y });
            setTextColor(settings.autoActivate ? ConsoleColor::Green : ConsoleColor::Red);
            std::cout << (settings.autoActivate ? "On" : "Off");
            setTextColor(ConsoleColor::Default);
            std::cout << std::flush;
            continue;
        }
        else if (i == 1) { // update rate (hz)
            std::ostringstream oss;
            oss << static_cast<int>(settings.updateRate) << " Hz";
            value = oss.str();
        }
        else if (i == 2) { // m_yaw
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << settings.m_yaw;
            value = oss.str();
        }
        else if (i == 3) { // cl_yawspeed
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << settings.cl_yawspeed;
            value = oss.str();
        }
        else if (i == 4) { // +left key
            value = keyToString(settings.leftKey);
        }
        else if (i == 5) { // +right key
            value = keyToString(settings.rightKey);
        }
        else if (i == 6) { // modifier key
            value = keyToString(settings.modifierKey);
        }
        else if (i == 7) { // modifier
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << settings.modifier;
            value = oss.str();
        }
        else if (i == 8) { // auto-activate toggle key
            value = keyToString(settings.autoActivateToggleKey);
        }

        if (i == selectedOption) {
            setTextColor(ConsoleColor::Red);
            writeAt(pos, prefix + options[i] + ": " + value, true);
            setTextColor(ConsoleColor::Default);
        }
        else {
            writeAt(pos, prefix + options[i] + ": " + value, true);
        }
    }
}

void ConsoleUI::updateStatusDisplay(bool running) {
    writeAt(statusPos_, "Status:", true);
    COORD valuePos = { 8, statusPos_.Y };
    writeAt(valuePos, "", true);
    SetConsoleCursorPosition(hConsole_, valuePos);
    setTextColor(running ? ConsoleColor::Green : ConsoleColor::Red);
    std::cout << (running ? "Running" : "Stopped");
    setTextColor(ConsoleColor::Default);
    std::cout << std::flush;
}

void ConsoleUI::updateCS2StatusDisplay(bool detected) {
    COORD cs2StatusPos = { 0, static_cast<SHORT>(statusPos_.Y + 1) };
    writeAt(cs2StatusPos, "Window Found:", true);
    COORD valuePos = { 14, cs2StatusPos.Y };
    writeAt(valuePos, "", true);
    SetConsoleCursorPosition(hConsole_, valuePos);
    setTextColor(detected ? ConsoleColor::Green : ConsoleColor::Red);
    std::cout << (detected ? "True" : "False");
    setTextColor(ConsoleColor::Default);
    std::cout << std::flush;
}

void ConsoleUI::updateDebugDisplay(const std::string& debugMsg, bool temporary) {
    std::string truncatedMsg = "Debug: " + debugMsg;
    if (truncatedMsg.length() > 30) {
        truncatedMsg = truncatedMsg.substr(0, 27) + "...";
    }
    lastDebugMsg_ = truncatedMsg;
    writeAt(debugPos_, lastDebugMsg_, true);
    if (temporary) {
        debugMsgActive_ = true;
        debugMsgTime_ = std::chrono::steady_clock::now();
    }
}

void ConsoleUI::updateInputDebugDisplay(const std::string& inputMsg) {
    lastInputMsg_ = "Input: " + inputMsg;
    writeAt(inputDebugPos_, lastInputMsg_, true);
}

void ConsoleUI::updateErrorDisplay(const std::string& errorMsg, bool temporary) {
    lastErrorMsg_ = "MSG: " + errorMsg;
    writeAt(errorPos_, lastErrorMsg_, true);
    if (temporary) {
        errorMsgActive_ = true;
        errorMsgTime_ = std::chrono::steady_clock::now();
    }
}

void ConsoleUI::clearTemporaryMessages() {
    auto now = std::chrono::steady_clock::now();
    if (debugMsgActive_ && std::chrono::duration_cast<std::chrono::milliseconds>(now - debugMsgTime_).count() >= 2000) {
        writeAt(debugPos_, "Debug: ", true);
        debugMsgActive_ = false;
    }
    if (errorMsgActive_ && std::chrono::duration_cast<std::chrono::milliseconds>(now - errorMsgTime_).count() >= 2000) {
        writeAt(errorPos_, "Error: ", true);
        errorMsgActive_ = false;
    }
}

void ConsoleUI::drawSeparatorLine(COORD pos) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole_, &csbi);
    std::string separator(csbi.dwSize.X - 1, '-');
    writeAt(pos, separator, false);
}

void ConsoleUI::displayInstructions() {
    printColored("ARROWS: Move | ", ConsoleColor::White);
    printColored("ENTER: Select Bind | ", ConsoleColor::White);
    printColored("Q: Quit\n", ConsoleColor::White);
    printColored("CONFIG:\n", ConsoleColor::Blue);
}

int ConsoleUI::detectKeyPress() {
    COORD tempPos = { 0, errorPos_.Y };
    updateErrorDisplay("Press a key or mouse button to assign...", true);
    Sleep(250);
    while (_kbhit()) _getch();
    while (true) {
        for (int vk = 0x01; vk <= 0xFE; ++vk) {
            if (vk == VK_RETURN) continue;
            if (GetAsyncKeyState(vk) & 0x8000) {
                while (GetAsyncKeyState(vk) & 0x8000) Sleep(10);
                updateErrorDisplay("Detected: " + keyToString(vk), true);
                Sleep(1000);
                updateErrorDisplay("", false);
                return vk;
            }
        }
        Sleep(10);
    }
}
