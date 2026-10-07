// Based on shizangle/shiz-turnbinds (MIT) - https://github.com/shizangle/shiz-turnbinds
// Added: configurable pause hotkey (default F8) that toggles the turnbinds while in game.

#include "ConsoleUI.h"
#include "Settings.h"
#include "MouseSimulation.h"
#include "CS2Monitor.h"
#include "Utils.h"
#include "FloatingUI.h"

#include <windows.h>
#include <mmsystem.h>
#include <conio.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#pragma comment(lib, "winmm.lib")

int main() {
    timeBeginPeriod(1);

    ConsoleUI ui;
    SettingsManager settings(exeDirFile("settings.json")); // lives next to the exe
    MouseSimulator simulator;

    std::atomic<bool> running{ false };
    std::atomic<bool> paused{ false };
    std::atomic<bool> rebinding{ false };   // true while the console is capturing a new key
    std::atomic<bool> shouldRunSim{ true };

    FloatingUI floatingUI(settings, paused);

    auto debugCallback = [&](const std::string& msg) {
        static auto lastDebugUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDebugUpdate).count() >= 500) {
            ui.updateDebugDisplay(msg);
            lastDebugUpdate = now;
        }
    };

    std::thread simThread([&]() { simulator.run(running, paused, shouldRunSim, settings, debugCallback); });
    std::thread monitorThread(monitorCS2, std::ref(running), std::ref(shouldRunSim), std::cref(settings));
    std::thread floatingUIThread([&]() { floatingUI.Show(); });

    // Pause hotkey: polled globally (no hooks, no admin), edge-triggered toggle.
    // Only reacts while CS2 is the focused window so the key stays usable elsewhere.
    std::thread hotkeyThread([&]() {
        bool wasDown = false;
        while (shouldRunSim) {
            auto cfg = settings.get();
            bool down = cfg.pauseKey != 0 && !rebinding &&
                        (GetAsyncKeyState(cfg.pauseKey) & 0x8000) != 0;
            if (down && !wasDown && isCS2WindowActive()) {
                paused = !paused;
            }
            wasDown = down;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    int selectedOption = 0;
    constexpr int kOptionCount = 9;
    bool lastRunning = false;
    bool lastPaused = false;
    bool lastCS2Status = false;
    bool lastLeftDown = false, lastRightDown = false;
    auto lastCS2Check = std::chrono::steady_clock::now();

    ui.displayInstructions();
    ui.updateSettingsDisplay(selectedOption, settings.get());
    ui.updateStatusDisplay(running, paused);
    ui.updateCS2StatusDisplay(isCS2WindowActive());

    while (shouldRunSim) {
        if (_kbhit()) {
            int ch = _getch();
            bool settingsChanged = false;
            std::string msg;

            if (ch == 224 || ch == 0) { // arrow keys
                ch = _getch();
                if (ch == 72)      selectedOption = (selectedOption - 1 + kOptionCount) % kOptionCount;
                else if (ch == 80) selectedOption = (selectedOption + 1) % kOptionCount;
                else if (ch == 75 || ch == 77) {
                    float delta = (ch == 75 ? -1.0f : 1.0f);
                    settings.update([&](SimulationSettings& s) {
                        if (selectedOption == 1) {
                            s.updateRate = clampf(s.updateRate + delta * 100.0f, 100.0f, 2000.0f);
                            msg = "Rate to " + std::to_string(static_cast<int>(s.updateRate));
                        }
                        else if (selectedOption == 2) {
                            s.m_yaw = clampf(s.m_yaw + delta * 0.001f, 0.001f, 0.1f);
                            msg = "m_yaw to " + std::to_string(s.m_yaw);
                        }
                        else if (selectedOption == 3) {
                            s.cl_yawspeed = clampf(s.cl_yawspeed + delta * 10.0f, 10.0f, 500.0f);
                            msg = "yawspeed to " + std::to_string(s.cl_yawspeed);
                        }
                        else if (selectedOption == 7) {
                            s.modifier = clampf(s.modifier + delta * 0.1f, 0.1f, 2.0f);
                            msg = "Modifier to " + std::to_string(s.modifier);
                        }
                    });
                    settingsChanged = true;
                }
            }
            else if (ch == 13) { // Enter
                if (selectedOption == 0) {
                    settings.update([&](SimulationSettings& s) {
                        s.autoActivate = !s.autoActivate;
                        msg = "Auto to " + std::string(s.autoActivate ? "On" : "Off");
                    });
                    settingsChanged = true;
                }
                else if ((selectedOption >= 4 && selectedOption <= 6) || selectedOption == 8) {
                    rebinding = true;
                    int newKey = ui.detectKeyPress();
                    rebinding = false;
                    if (newKey != 0) {
                        settings.update([&](SimulationSettings& s) {
                            if (selectedOption == 4)      { s.leftKey = newKey;     msg = "Left Key to " + keyToString(newKey); }
                            else if (selectedOption == 5) { s.rightKey = newKey;    msg = "Right Key to " + keyToString(newKey); }
                            else if (selectedOption == 6) { s.modifierKey = newKey; msg = "Mod Key to " + keyToString(newKey); }
                            else if (selectedOption == 8) { s.pauseKey = newKey;    msg = "Pause Key to " + keyToString(newKey); }
                        });
                        settingsChanged = true;
                    }
                }
            }
            else if (ch == 'q' || ch == 'Q') {
                shouldRunSim = false;
                running = false;
                break;
            }

            if (!msg.empty()) ui.updateDebugDisplay(msg);
            if (settingsChanged) settings.save();
            ui.updateSettingsDisplay(selectedOption, settings.get());
        }

        if (lastRunning != running || lastPaused != paused) {
            ui.updateStatusDisplay(running, paused);
            if (lastPaused != paused) ui.updateDebugDisplay(paused ? "Paused" : "Resumed");
            lastRunning = running;
            lastPaused = paused;
        }

        auto cfg = settings.get();
        bool leftDown = (GetAsyncKeyState(cfg.leftKey) & 0x8000) != 0;
        bool rightDown = (GetAsyncKeyState(cfg.rightKey) & 0x8000) != 0;
        if (leftDown != lastLeftDown || rightDown != lastRightDown) {
            std::string inputMsg;
            if (leftDown && !rightDown)      inputMsg = keyToString(cfg.leftKey);
            else if (rightDown && !leftDown) inputMsg = keyToString(cfg.rightKey);
            else if (leftDown && rightDown)  inputMsg = keyToString(cfg.leftKey) + " + " + keyToString(cfg.rightKey);
            else                             inputMsg = "None";
            ui.updateInputDebugDisplay(inputMsg);
            lastLeftDown = leftDown;
            lastRightDown = rightDown;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCS2Check).count() >= 500) {
            bool cs2 = isCS2WindowActive();
            if (cs2 != lastCS2Status) {
                ui.updateCS2StatusDisplay(cs2);
                lastCS2Status = cs2;
            }
            lastCS2Check = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    shouldRunSim = false;
    floatingUI.Close();
    if (simThread.joinable()) simThread.join();
    if (monitorThread.joinable()) monitorThread.join();
    if (hotkeyThread.joinable()) hotkeyThread.join();
    if (floatingUIThread.joinable()) floatingUIThread.join();

    timeEndPeriod(1);
    return 0;
}
