#pragma once
#include "Settings.h"
#include <atomic>
#include <functional>
#include <string>

class MouseSimulator {
public:
    // Runs until shouldRun becomes false.
    // Does nothing (and resets its state) while !running or while paused.
    void run(std::atomic<bool>& running,
             const std::atomic<bool>& paused,
             const std::atomic<bool>& shouldRun,
             const SettingsManager& settings,
             std::function<void(const std::string&)> debugCallback);

private:
    void moveMouse(int deltaX);
};
