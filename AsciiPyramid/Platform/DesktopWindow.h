#pragma once
#include "Win32Interop.h"
#include <cstdio>

namespace Platform {

// Creates an owned child window we can bind DComp to. We can't bind DComp
// directly to WorkerW (cross-process E_ACCESSDENIED), but we *can* create
// our own borderless full-screen window, set WorkerW as its parent, and
// bind DComp to ours. The window inherits WorkerW's z-order position
// (behind icons, in front of wallpaper).
class DesktopWindow {
public:
    HWND hwnd = nullptr;
    HWND attachedTo = nullptr;
    int  screenW = 0;
    int  screenH = 0;

    static const wchar_t* ClassName() { return L"AsciiPyramidWindow"; }

    bool Create() {
        screenW = ::GetSystemMetrics(SM_CXSCREEN);
        screenH = ::GetSystemMetrics(SM_CYSCREEN);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.lpszClassName = ClassName();
        wc.hCursor = ::LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = nullptr;
        ::RegisterClassExW(&wc); // ignore "already registered" errors

        // WS_POPUP + WS_VISIBLE; no WS_EX_TOPMOST (we want WorkerW's z position).
        // WS_EX_NOACTIVATE so clicks pass through to icons/desktop above.
        // WS_EX_TRANSPARENT lets mouse events fall through.
        // WS_EX_NOREDIRECTIONBITMAP is required for DComp-only swap chains.
        DWORD ex = WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED |
                   WS_EX_NOREDIRECTIONBITMAP;
        DWORD style = WS_POPUP | WS_VISIBLE;

        // Note: WS_EX_LAYERED + WS_EX_NOREDIRECTIONBITMAP are mutually
        // problematic on some Win versions. Use NOREDIRECTIONBITMAP only;
        // remove LAYERED. NOREDIRECTIONBITMAP is what DComp actually wants.
        ex = WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP;

        hwnd = ::CreateWindowExW(
            ex, ClassName(), L"AsciiPyramid",
            style,
            0, 0, screenW, screenH,
            nullptr, // parent set later in AttachToWorkerW
            nullptr, wc.hInstance, nullptr);

        if (!hwnd) {
            std::printf("[diag] CreateWindowExW failed: %lu\n", ::GetLastError());
            return false;
        }
        return true;
    }

    bool AttachToWorkerW(HWND workerW) {
        attachedTo = workerW;
        if (!hwnd || !workerW) return false;

        // Reparent our window under WorkerW. Z-order follows WorkerW.
        HWND prev = ::SetParent(hwnd, workerW);
        if (!prev && ::GetLastError() != 0) {
            std::printf("[diag] SetParent failed: %lu\n", ::GetLastError());
        }

        // Resize to fill the WorkerW client area.
        RECT wr{};
        ::GetClientRect(workerW, &wr);
        if (wr.right > 0 && wr.bottom > 0) {
            screenW = wr.right;
            screenH = wr.bottom;
        }
        ::SetWindowPos(hwnd, nullptr, 0, 0, screenW, screenH,
                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        return true;
    }

    bool IsTargetValid() const {
        return attachedTo && ::IsWindow(attachedTo) && hwnd && ::IsWindow(hwnd);
    }

    ~DesktopWindow() {
        if (hwnd) ::DestroyWindow(hwnd);
    }
};

} // namespace Platform