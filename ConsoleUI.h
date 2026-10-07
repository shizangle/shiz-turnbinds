#pragma once
#include "Settings.h"
#include <mutex>
#include <string>

enum class ConsoleColor { White, Green, Red, Yellow, Cyan, Gray };

class ConsoleUI {
public:
    ConsoleUI();
    ~ConsoleUI();

    void printColored(const std::string& text, ConsoleColor color);
    void displayInstructions();
    void updateSettingsDisplay(int selected, const SimulationSettings& s);
    void updateStatusDisplay(bool running, bool paused);
    void updateCS2StatusDisplay(bool cs2Active);
    void updateDebugDisplay(const std::string& msg);
    void updateInputDebugDisplay(const std::string& msg);

    // Blocks until a key/mouse button is pressed; returns its VK code.
    // Returns 0 if cancelled with Esc.
    int detectKeyPress();

private:
    void redraw();

    std::mutex mutex_;
    SimulationSettings settings_;
    int selected_ = 0;
    bool running_ = false;
    bool paused_ = false;
    bool cs2Active_ = false;
    bool prompting_ = false;
    std::string debug_;
    std::string input_ = "None";
};
