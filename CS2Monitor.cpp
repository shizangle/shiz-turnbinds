#include "CS2Monitor.h"
#include "Utils.h"
#include <thread>
#include <chrono>

void monitorCS2(std::atomic<bool>& running, std::atomic<bool>& shouldRun, const SettingsManager& settings) {
    bool lastActive = false;
    while (shouldRun) {
        bool cs2Active = isCS2WindowActive();
        auto cfg = settings.get();
        if (cfg.autoActivate) {
            if (cs2Active && !lastActive) running = true;
            else if (!cs2Active && lastActive) running = false;
        }
        lastActive = cs2Active;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
