#include "Utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <string>

long long performance_counter_frequency() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return f.QuadPart;
}

long long performance_counter() {
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
}

std::string keyToString(int vk) {
    switch (vk) {
    case 0:    return "None";
    case 0x01: return "Mouse1";
    case 0x02: return "Mouse2";
    case 0x04: return "Mouse3";
    case 0x05: return "Mouse5";
    case 0x06: return "Mouse4";
    case 0x10: return "Shift";
    case 0x11: return "Ctrl";
    case 0x12: return "Alt";
    }
    if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, static_cast<char>(vk));

    UINT sc = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    LONG lParam = static_cast<LONG>(sc << 16);
    switch (vk) { // extended keys need bit 24 set to resolve their name
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT: case VK_HOME: case VK_END:
    case VK_INSERT: case VK_DELETE: case VK_NUMLOCK:
        lParam |= (1 << 24);
    }
    char buf[64] = {};
    if (sc != 0 && GetKeyNameTextA(lParam, buf, sizeof(buf)) > 0) return buf;
    return "VK_0x" + std::to_string(vk);
}

float clampf(float value, float lo, float hi) {
    return std::max(lo, std::min(hi, value));
}

bool isCS2WindowActive() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    char windowTitle[256] = {};
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    return std::string(windowTitle).find("Counter-Strike") != std::string::npos;
}

std::string exeDirFile(const std::string& name) {
    char path[MAX_PATH] = {};
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return name;
    std::string dir(path, n);
    size_t slash = dir.find_last_of("\\/");
    if (slash == std::string::npos) return name;
    return dir.substr(0, slash + 1) + name;
}
