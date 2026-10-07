#pragma once
#include "Settings.h"
#include <atomic>

// Auto-activates / deactivates the sim when CS2 gains / loses focus.
void monitorCS2(std::atomic<bool>& running, std::atomic<bool>& shouldRun, const SettingsManager& settings);
