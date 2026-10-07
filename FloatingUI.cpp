#include "FloatingUI.h"
#include "Utils.h"
#include <windowsx.h>
#include <cstdio>

namespace {
constexpr int kWidth = 150;
constexpr int kHeight = 46;
constexpr UINT_PTR kTimerId = 1;
}

FloatingUI::FloatingUI(SettingsManager& settings, const std::atomic<bool>& paused)
    : settings_(settings), paused_(paused) {}

void FloatingUI::Show() {
    HINSTANCE inst = GetModuleHandle(nullptr);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_SIZEALL);
    wc.lpszClassName = "ShizTurnbindsOverlay";
    RegisterClassA(&wc);

    auto cfg = settings_.get();
    int x = cfg.windowX, y = cfg.windowY;
    if (x < 0 || y < 0) {
        x = (GetSystemMetrics(SM_CXSCREEN) - kWidth) / 2;
        y = 40;
    }

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        wc.lpszClassName, "Shiz-Turnbinds", WS_POPUP,
        x, y, kWidth, kHeight, nullptr, nullptr, inst, this);
    if (!hwnd) return;

    hwnd_ = hwnd;
    SetLayeredWindowAttributes(hwnd, 0, 225, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    SetTimer(hwnd, kTimerId, 100, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    hwnd_ = nullptr;
}

void FloatingUI::Close() {
    HWND h = hwnd_.load();
    if (h) PostMessage(h, WM_CLOSE, 0, 0);
}

LRESULT CALLBACK FloatingUI::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    FloatingUI* self = nullptr;
    if (msg == WM_NCCREATE) {
        self = static_cast<FloatingUI*>(reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else {
        self = reinterpret_cast<FloatingUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) return self->handle(hwnd, msg, wp, lp);
    return DefWindowProc(hwnd, msg, wp, lp);
}

LRESULT FloatingUI::handle(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE; // never steal focus from the game

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        dragging_ = true;
        moved_ = false;
        GetCursorPos(&dragCursorStart_);
        RECT r; GetWindowRect(hwnd, &r);
        dragWindowStart_ = { r.left, r.top };
        return 0;
    }
    case WM_MOUSEMOVE:
        if (dragging_) {
            POINT p; GetCursorPos(&p);
            int nx = dragWindowStart_.x + (p.x - dragCursorStart_.x);
            int ny = dragWindowStart_.y + (p.y - dragCursorStart_.y);
            SetWindowPos(hwnd, HWND_TOPMOST, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            moved_ = true;
        }
        return 0;

    case WM_LBUTTONUP:
        if (dragging_) {
            ReleaseCapture();
            dragging_ = false;
            if (moved_) {
                RECT r; GetWindowRect(hwnd, &r);
                settings_.update([&](SimulationSettings& s) { s.windowX = r.left; s.windowY = r.top; });
                settings_.save();
            }
        }
        return 0;

    case WM_MOUSEWHEEL: {
        int steps = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
        if (steps != 0) {
            settings_.update([&](SimulationSettings& s) {
                s.cl_yawspeed = clampf(s.cl_yawspeed + steps * 10.0f, 10.0f, 500.0f);
            });
            settings_.save();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_TIMER: {
        float ys = settings_.get().cl_yawspeed;
        int p = paused_ ? 1 : 0;
        if (ys != drawnYawSpeed_ || p != drawnPaused_) InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        float ys = settings_.get().cl_yawspeed;
        bool paused = paused_;
        drawnYawSpeed_ = ys;
        drawnPaused_ = paused ? 1 : 0;

        HBRUSH bg = CreateSolidBrush(paused ? RGB(70, 55, 10) : RGB(25, 30, 35));
        FillRect(dc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(dc, TRANSPARENT);
        HFONT font = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HGDIOBJ old = SelectObject(dc, font);

        char l1[64];
        std::snprintf(l1, sizeof(l1), "yawspeed  %g", ys);
        RECT r1 = rc; r1.bottom = rc.top + kHeight / 2; r1.top += 4;
        SetTextColor(dc, RGB(235, 235, 235));
        DrawTextA(dc, l1, -1, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT r2 = rc; r2.top = rc.top + kHeight / 2 - 2;
        SetTextColor(dc, paused ? RGB(255, 200, 60) : RGB(110, 220, 120));
        DrawTextA(dc, paused ? "PAUSED" : "ACTIVE", -1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(dc, old);
        DeleteObject(font);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, kTimerId);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
