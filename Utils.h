#pragma once
#include <string>

long long performance_counter_frequency();
long long performance_counter();
std::string keyToString(int vkCode);
float clampf(float value, float lo, float hi);
bool isCS2WindowActive();
std::string exeDirFile(const std::string& name); // path next to the running exe
