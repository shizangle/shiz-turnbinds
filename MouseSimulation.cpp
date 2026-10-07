#include "MouseSimulation.h"
#include "Utils.h"
#include <windows.h>
#include <chrono>
#include <cmath>
#include <sstream>
#include <thread>

void MouseSimulator::run(std::atomic<bool>& running,
                         const std::atomic<bool>& paused,
                         const std::atomic<bool>& shouldRun,
                         const SettingsManager& settings,
                         std::function<void(const std::string&)> debugCallback) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    double remaining = 0.0;
    double currentSpeed = 0.0;
    long long lastTime = performance_counter();
    bool lastLeft = false, lastRight = false;

    while (shouldRun) {
        // Inactive or paused: idle and drop all motion state
        if (!running || paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            currentSpeed = 0.0;
            remaining = 0.0;
            lastTime = performance_counter();
            debugCallback("");
            continue;
        }
        if (!isCS2WindowActive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            lastTime = performance_counter();
            continue;
        }

        auto cfg = settings.get();
        bool leftDown     = (GetAsyncKeyState(cfg.leftKey) & 0x8000) != 0;
        bool rightDown    = (GetAsyncKeyState(cfg.rightKey) & 0x8000) != 0;
        bool modifierDown = (GetAsyncKeyState(cfg.modifierKey) & 0x8000) != 0;

        int direction = 0;
        std::string keyStatus;
        if (leftDown && !rightDown)      { direction = -1; keyStatus = keyToString(cfg.leftKey); }
        else if (rightDown && !leftDown) { direction = 1;  keyStatus = keyToString(cfg.rightKey); }
        else if (leftDown && rightDown)  { direction = 0;  keyStatus = "Both keys pressed"; }

        long long currentTime = performance_counter();
        double deltaTime = static_cast<double>(currentTime - lastTime) / performance_counter_frequency();
        lastTime = currentTime;

        if (leftDown != lastLeft || rightDown != lastRight || deltaTime >= (1.0 / cfg.updateRate)) {
            if (direction != 0) {
                double effectiveYawSpeed = cfg.cl_yawspeed * (modifierDown ? cfg.modifier : 1.0);
                double targetSpeed = direction * effectiveYawSpeed * (1.0 / cfg.m_yaw);
                currentSpeed += (targetSpeed - currentSpeed) * 0.15; // smooth ramp

                double moveAmount = currentSpeed * deltaTime;
                if (std::abs(moveAmount) > 100.0) moveAmount = std::copysign(100.0, moveAmount);

                remaining += moveAmount;
                int intMove = static_cast<int>(std::round(remaining));
                if (std::abs(intMove) >= 1) {
                    moveMouse(intMove);
                    remaining -= intMove;
                }

                std::ostringstream oss;
                oss << keyStatus << " | m_yaw: " << cfg.m_yaw
                    << ", cl_yawspeed: " << effectiveYawSpeed
                    << ", move: " << moveAmount
                    << ", remaining: " << remaining
                    << ", modifier: " << (modifierDown ? "ON" : "OFF");
                debugCallback(oss.str());
            }
            else {
                currentSpeed = 0.0;
                remaining = 0.0;
                debugCallback("");
            }
        }

        lastLeft = leftDown;
        lastRight = rightDown;
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(1e6 / cfg.updateRate)));
    }
}

void MouseSimulator::moveMouse(int deltaX) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = deltaX;
    SendInput(1, &input, sizeof(INPUT));
}
