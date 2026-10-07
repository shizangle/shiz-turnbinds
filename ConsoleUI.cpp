#include "ConsoleUI.h"
#include "Utils.h"
#include "Banner.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace {
const char* ansi(ConsoleColor c) {
    switch (c) {
    case ConsoleColor::Green:  return "\x1b[92m";
    case ConsoleColor::Red:    return "\x1b[91m";
    case ConsoleColor::Yellow: return "\x1b[93m";
    case ConsoleColor::Cyan:   return "\x1b[96m";
    case ConsoleColor::Gray:   return "\x1b[90m";
    default:                   return "\x1b[97m";
    }
}
const char* RESET = "\x1b[0m";
const char* CLR = "\x1b[K"; // clear to end of line

// Visible width of a UTF-8 string, ignoring ANSI colour sequences
size_t visLen(const std::string& str) {
    size_t n = 0; bool esc = false;
    for (unsigned char c : str) {
        if (esc) { if (c == 'm') esc = false; continue; }
        if (c == 0x1b) { esc = true; continue; }
        if ((c & 0xC0) != 0x80) ++n;
    }
    return n;
}
std::string padTo(const std::string& str, size_t w) {
    size_t v = visLen(str);
    return v < w ? str + std::string(w - v, ' ') : str;
}
std::string rep(const char* unit, int n) {
    std::string r; for (int i = 0; i < n; ++i) r += unit; return r;
}

std::string fg256(int n) { return "\x1b[38;5;" + std::to_string(n) + "m"; }

std::string fmt(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);
    return buf;
}
}

ConsoleUI::ConsoleUI() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);                       // block characters
    SetConsoleTitleA("Shiz-Turnbinds");
    std::cout << "\x1b[?1049h\x1b[2J\x1b[3J\x1b[H\x1b[?25l" << std::flush; // alt screen, clear, hide cursor
}

ConsoleUI::~ConsoleUI() {
    std::cout << "\x1b[?25h" << RESET << "\x1b[?1049l" << std::flush; // show cursor, leave alt screen
}

void ConsoleUI::printColored(const std::string&, ConsoleColor) {
    // Banner is part of redraw() so it stays in place; kept for API parity.
}

void ConsoleUI::displayInstructions() { redraw(); }

void ConsoleUI::updateSettingsDisplay(int selected, const SimulationSettings& s) {
    { std::lock_guard<std::mutex> l(mutex_); selected_ = selected; settings_ = s; }
    redraw();
}
void ConsoleUI::updateStatusDisplay(bool running, bool paused) {
    { std::lock_guard<std::mutex> l(mutex_); running_ = running; paused_ = paused; }
    redraw();
}
void ConsoleUI::updateCS2StatusDisplay(bool a) {
    { std::lock_guard<std::mutex> l(mutex_); cs2Active_ = a; }
    redraw();
}
void ConsoleUI::updateDebugDisplay(const std::string& m) {
    { std::lock_guard<std::mutex> l(mutex_); debug_ = m; }
    redraw();
}
void ConsoleUI::updateInputDebugDisplay(const std::string& m) {
    { std::lock_guard<std::mutex> l(mutex_); input_ = m; }
    redraw();
}

void ConsoleUI::redraw() {
    std::lock_guard<std::mutex> l(mutex_);
    const SimulationSettings& s = settings_;

    // ---- measure the window so we never print more than fits (scrolling = banner spam) ----
    int cols = 120, rows = 30;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    const std::string G = ansi(ConsoleColor::Gray), C = ansi(ConsoleColor::Cyan), R = RESET;
    const std::string kUrlShiz = "https://discord.com/invite/V9KhKrJ3Pf";
    const std::string kUrlLow  = "https://flowstatecs.com/discord";
    static const int kShizGrad[6] = { 51, 45, 39, 33, 27, 21 };      // cyan -> blue
    static const int kLowGrad[6]  = { 213, 207, 201, 165, 129, 93 }; // pink -> purple
    const size_t kArtW = 47;

    // ---- everything below the banner ----
    struct Row { const char* label; std::string value; };
    const Row rowsTbl[] = {
        { "Auto activate", s.autoActivate ? "On" : "Off" },
        { "Update rate",   fmt(s.updateRate) },
        { "m_yaw",         fmt(s.m_yaw) },
        { "cl_yawspeed",   fmt(s.cl_yawspeed) },
        { "Left key",      keyToString(s.leftKey) },
        { "Right key",     keyToString(s.rightKey) },
        { "Modifier key",  keyToString(s.modifierKey) },
        { "Modifier",      fmt(s.modifier) },
        { "Pause key",     keyToString(s.pauseKey) },
    };

    std::vector<std::string> rest;
    rest.push_back(" Up/Down select | Left/Right adjust | Enter toggle/rebind | Q quit");
    rest.push_back("");
    for (int i = 0; i < 9; ++i) {
        bool sel = (i == selected_);
        char line[96];
        std::snprintf(line, sizeof(line), "%-14s %s", rowsTbl[i].label, rowsTbl[i].value.c_str());
        rest.push_back(std::string(sel ? ansi(ConsoleColor::Cyan) : ansi(ConsoleColor::White))
                       + (sel ? " > " : "   ") + line + R);
    }
    rest.push_back("");
    if (paused_)
        rest.push_back(std::string(ansi(ConsoleColor::Yellow)) + " Status : PAUSED (press " + keyToString(s.pauseKey) + " to resume)" + R);
    else if (running_)
        rest.push_back(std::string(ansi(ConsoleColor::Green)) + " Status : RUNNING" + R);
    else
        rest.push_back(std::string(ansi(ConsoleColor::Red)) + " Status : STOPPED" + R);
    rest.push_back(std::string(cs2Active_ ? ansi(ConsoleColor::Green) : ansi(ConsoleColor::Gray))
                   + " CS2    : " + (cs2Active_ ? "in focus" : "not in focus") + R);
    rest.push_back(" Input  : " + input_);
    if (prompting_)
        rest.push_back(std::string(ansi(ConsoleColor::Yellow)) + " Press the new key / mouse button (Esc to cancel)..." + R);
    else
        rest.push_back(G + " " + debug_ + R);

    // ---- banner variants, widest/tallest first ----
    // 1) stacked: Made by / SHIZ / <> / Maintained / LOW
    std::vector<std::string> stacked;
    stacked.push_back(G + "  Made by" + R);
    for (int i = 0; i < 6; ++i) stacked.push_back(fg256(kShizGrad[i]) + "  " + kShizArt[i] + R);
    stacked.push_back(G + "  discord - " + C + kUrlShiz + R);
    stacked.push_back(G + "  " + rep("\xE2\x94\x80", 22) + " " + R + "<>" + G + " " + rep("\xE2\x94\x80", 22) + R);
    stacked.push_back(G + "  Maintained" + R);
    for (int i = 0; i < 6; ++i) stacked.push_back(fg256(kLowGrad[i]) + "  " + kLowArt[i] + R);
    stacked.push_back(G + "  discord - " + C + kUrlLow + R);
    stacked.push_back("");

    // 2) side by side: shorter, needs a wide window
    std::vector<std::string> side;
    side.push_back(padTo(G + "  Made by" + R, 2 + kArtW + 6) + G + "Maintained" + R);
    for (int i = 0; i < 6; ++i) {
        std::string left  = fg256(kShizGrad[i]) + "  " + kShizArt[i] + R;
        std::string mid   = (i == 2) ? "  <>  " : "      ";
        side.push_back(padTo(left, 2 + kArtW) + mid + fg256(kLowGrad[i]) + kLowArt[i] + R);
    }
    side.push_back(padTo(G + "  discord - " + C + kUrlShiz + R, 2 + kArtW + 6) + G + "discord - " + C + kUrlLow + R);
    side.push_back("");

    // 3) plain text fallback for small windows
    std::vector<std::string> compact;
    compact.push_back(G + "  Made by Shiz  <>  Maintained by Low" + R);
    compact.push_back(G + "  " + C + kUrlShiz + R);
    compact.push_back(G + "  " + C + kUrlLow + R);
    compact.push_back("");

    auto fits = [&](const std::vector<std::string>& b, int needCols) {
        return cols >= needCols && static_cast<int>(b.size() + rest.size()) <= rows - 1;
    };
    const std::vector<std::string>* banner = &compact;
    if (fits(stacked, 52))                 banner = &stacked;
    else if (fits(side, 2 + (int)kArtW + 6 + (int)kArtW + 2)) banner = &side;

    std::vector<std::string> lines = *banner;
    lines.insert(lines.end(), rest.begin(), rest.end());
    if (static_cast<int>(lines.size()) > rows - 1) lines.resize(std::max(1, rows - 1)); // never scroll

    std::string out = "\x1b[H";
    for (size_t i = 0; i < lines.size(); ++i) {
        out += lines[i];
        out += CLR;
        if (i + 1 < lines.size()) out += "\n";
    }
    out += "\x1b[J"; // clear anything left below
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

int ConsoleUI::detectKeyPress() {
    { std::lock_guard<std::mutex> l(mutex_); prompting_ = true; }
    redraw();

    // Wait for Enter and the mouse buttons to be released so the rebind
    // doesn't instantly capture whatever was held when it started
    while ((GetAsyncKeyState(VK_RETURN) & 0x8000) ||
           (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
           (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    int result = 0;
    bool done = false;
    while (!done) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { result = 0; done = true; break; }
        for (int vk = 1; vk < 255; ++vk) {
            if (vk == VK_RETURN || vk == VK_ESCAPE) continue;
            if (GetAsyncKeyState(vk) & 0x8000) { result = vk; done = true; break; }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    // Wait for release so the key press doesn't leak into the game/menu
    while (result && (GetAsyncKeyState(result) & 0x8000)) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    { std::lock_guard<std::mutex> l(mutex_); prompting_ = false; }
    redraw();
    return result;
}
