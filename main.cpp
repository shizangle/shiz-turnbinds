#include "ConsoleUI.h"
#include "Settings.h"
#include "MouseSimulation.h"
#include "CS2Monitor.h"
#include "Utils.h"
#include "FloatingUI.h"
#include <thread>
#include <atomic>
#include <mmsystem.h>
#include <conio.h>
#include <mutex>
#include <string>
#include <iostream>

#pragma comment(lib, "winmm.lib")

int main() {
    timeBeginPeriod(1);

    ConsoleUI ui;
    SettingsManager settings;
    MouseSimulator simulator;
    FloatingUI floatingUI(settings);

    std::atomic<bool> running{ false };
    std::atomic<bool> shouldRunSim{ true };

    std::string debugMsg;
    std::mutex debugMutex;

    auto debugCallback = [&](const std::string& msg) {
        static auto lastDebugUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDebugUpdate).count() >= 500) {
            std::lock_guard<std::mutex> lock(debugMutex);
            debugMsg = msg;
            ui.updateDebugDisplay(debugMsg);
            lastDebugUpdate = now;
        }
        };

    std::thread simThread([&]() {
        simulator.run(running, settings, debugCallback);
        });

    std::thread monitorThread(monitorCS2, std::ref(running), std::ref(shouldRunSim), std::cref(settings));

    std::thread floatingUIThread([&]() {
        floatingUI.Show();
        });

    ui.printColored(R"(
 _  _  __  __  _  __  __  __    ___ _  _ ___ ___
| || |/  \|  \| |/ _]/  |/  \  /'_/| || | _ \ __|
| >< | /\ | | ' | [/\`7 | // |_`._`| \/ | v / _|
|_||_|_||_|_|\__|\__/ |_|\__/\|___/ \__/|_|_\_|

    turnbinds by Shizangle @ hang10.surf/discord

)", ConsoleColor::White);

    ui.displayInstructions();

    int selectedOption = 0;
    bool lastRunning = false;
    bool lastCS2Status = false;
    auto lastCS2Check = std::chrono::steady_clock::now();
    bool lastLeftDown = false;
    bool lastRightDown = false;
    bool lastAutoToggleKeyDown = false; // For debounce

    ui.updateSettingsDisplay(selectedOption, settings.get());
    ui.updateStatusDisplay(running);
    ui.updateCS2StatusDisplay(isCS2WindowActive());

    while (shouldRunSim) {
        if (_kbhit()) {
            int ch = _getch();
            bool settingsChanged = false;
            std::string settingChangeMsg;

            if (ch == 224) {
                ch = _getch();
                if (ch == 72) selectedOption = (selectedOption - 1 + 9) % 9; // 9 options now
                else if (ch == 80) selectedOption = (selectedOption + 1) % 9;
                else if (ch == 75 || ch == 77) {
                    float delta = (ch == 75 ? -1 : 1);
                    settings.update([&](SimulationSettings& s) {
                        if (selectedOption == 1) {
                            s.updateRate = clampf(s.updateRate + delta * 100.0f, 100.0f, 2000.0f);
                            settingChangeMsg = "Rate to " + std::to_string(static_cast<int>(s.updateRate));
                        }
                        else if (selectedOption == 2) {
                            s.m_yaw = clampf(s.m_yaw + delta * 0.001f, 0.001f, 0.1f);
                            settingChangeMsg = "m_yaw to " + std::to_string(s.m_yaw);
                        }
                        else if (selectedOption == 3) {
                            s.cl_yawspeed = clampf(s.cl_yawspeed + delta * 10.0f, 10.0f, 500.0f);
                            settingChangeMsg = "yawspeed to " + std::to_string(s.cl_yawspeed);
                        }
                        else if (selectedOption == 7) {
                            s.modifier = clampf(s.modifier + delta * 0.1f, 0.1f, 2.0f);
                            settingChangeMsg = "Modifier to " + std::to_string(s.modifier);
                        }
                        });
                    settingsChanged = true;
                    if (!settingChangeMsg.empty()) ui.updateDebugDisplay(settingChangeMsg);
                }
            }
            else if (ch == 13) {
                if (selectedOption == 0) {
                    settings.update([&](SimulationSettings& s) {
                        s.autoActivate = !s.autoActivate;
                        settingChangeMsg = "Auto to " + std::string(s.autoActivate ? "On" : "Off");
                        });
                    settingsChanged = true;
                }
                else if (selectedOption >= 4 && selectedOption <= 6) {
                    int newKey = ui.detectKeyPress();
                    settings.update([&](SimulationSettings& s) {
                        if (selectedOption == 4) {
                            s.leftKey = newKey;
                            settingChangeMsg = "Left Key to " + keyToString(newKey);
                        }
                        else if (selectedOption == 5) {
                            s.rightKey = newKey;
                            settingChangeMsg = "Right Key to " + keyToString(newKey);
                        }
                        else if (selectedOption == 6) {
                            s.modifierKey = newKey;
                            settingChangeMsg = "Mod Key to " + keyToString(newKey);
                        }
                        });
                    settingsChanged = true;
                }
                else if (selectedOption == 8) { // auto-activate toggle key
                    int newKey = ui.detectKeyPress();
                    settings.update([&](SimulationSettings& s) {
                        s.autoActivateToggleKey = newKey;
                        settingChangeMsg = "Auto-Activate Toggle Key to " + keyToString(newKey);
                        });
                    settingsChanged = true;
                }
                if (!settingChangeMsg.empty()) ui.updateDebugDisplay(settingChangeMsg);
            }
            else if (ch == 'q' || ch == 'Q') {
                shouldRunSim = false;
                running = false;
                break;
            }

            if (settingsChanged) settings.save();
            ui.updateSettingsDisplay(selectedOption, settings.get());
        }

        // --- auto-activate toggle key handling ---
        auto cfg = settings.get();
        SHORT keyState = GetAsyncKeyState(cfg.autoActivateToggleKey);
        bool autoToggleKeyDown = (keyState & 0x8000) != 0;
        if (autoToggleKeyDown && !lastAutoToggleKeyDown) {
            bool willBe = !cfg.autoActivate;
            settings.update([&](SimulationSettings& s) {
                s.autoActivate = willBe;
                });
            settings.save();
            ui.updateDebugDisplay(std::string("Auto-Activate toggled ") + (willBe ? "On" : "Off"));
            ui.updateSettingsDisplay(selectedOption, settings.get());
        }
        lastAutoToggleKeyDown = autoToggleKeyDown;
        // -----------------------------------------

        if (lastRunning != running) {
            ui.updateStatusDisplay(running);
            lastRunning = running;
        }

        bool leftDown = (GetAsyncKeyState(cfg.leftKey) & 0x8000) != 0;
        bool rightDown = (GetAsyncKeyState(cfg.rightKey) & 0x8000) != 0;
        if (leftDown != lastLeftDown || rightDown != lastRightDown) {
            std::string inputMsg;
            if (leftDown && !rightDown) inputMsg = keyToString(cfg.leftKey);
            else if (rightDown && !leftDown) inputMsg = keyToString(cfg.rightKey);
            else if (leftDown && rightDown) inputMsg = keyToString(cfg.leftKey) + " + " + keyToString(cfg.rightKey);
            else inputMsg = "None";
            ui.updateInputDebugDisplay(inputMsg);
            lastLeftDown = leftDown;
            lastRightDown = rightDown;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCS2Check).count() >= 500) {
            bool currentCS2Status = isCS2WindowActive();
            if (currentCS2Status != lastCS2Status) {
                ui.updateCS2StatusDisplay(currentCS2Status);
                lastCS2Status = currentCS2Status;
            }
            lastCS2Check = now;
        }

        ui.clearTemporaryMessages();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    PostMessage(HWND_BROADCAST, WM_QUIT, 0, 0);

    if (simThread.joinable()) simThread.join();
    if (monitorThread.joinable()) monitorThread.join();
    if (floatingUIThread.joinable()) floatingUIThread.join();

    std::cout << "Press Enter to exit...\n";
    std::cin.get();
    timeEndPeriod(1);
    return 0;
}
