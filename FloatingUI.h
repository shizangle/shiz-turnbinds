#pragma once
#include "Settings.h"
#include <atomic>
#include <windows.h>

// Small always-on-top, non-activating overlay:
//  - mouse wheel over it: cl_yawspeed +/- 10
//  - left-drag: move it (position is saved)
//  - shows PAUSED / ACTIVE state
class FloatingUI {
public:
    FloatingUI(SettingsManager& settings, const std::atomic<bool>& paused);
    void Show();   // blocks: creates the window and runs its message loop
    void Close();  // thread-safe

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(HWND, UINT, WPARAM, LPARAM);

    SettingsManager& settings_;
    const std::atomic<bool>& paused_;
    std::atomic<HWND> hwnd_{ nullptr };

    bool dragging_ = false;
    bool moved_ = false;
    POINT dragCursorStart_{};
    POINT dragWindowStart_{};

    float drawnYawSpeed_ = -1.0f;
    int drawnPaused_ = -1;
};
