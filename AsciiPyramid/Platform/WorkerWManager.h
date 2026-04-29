#pragma once
#include "Win32Interop.h"
#include <cstdio>
#include <cstring>

namespace Platform {

// Locates a WorkerW we can parent our DComp visual to.
// On Win11 24H2 the visible-wallpaper WorkerW is typically a child of Progman.
class WorkerWManager {
public:
    static HWND GetWorkerW() {
        HWND progman = ::FindWindowA("Progman", nullptr);
        if (!progman) return nullptr;

        const WPARAM kTrials[] = { 0x0000000D, 0x0000000A, 0x00000000 };
        for (WPARAM w : kTrials) {
            DWORD_PTR ig = 0;
            ::SendMessageTimeoutA(progman, 0x052C, w, 0, SMTO_NORMAL, 1000, &ig);
            ::Sleep(80);
            if (HWND found = Locate(progman)) return found;
        }
        return progman;
    }

private:
    static HWND Locate(HWND progman) {
        HWND result = nullptr;

        // Sibling pattern (Win10 layout).
        ::EnumWindows([](HWND top, LPARAM lp) -> BOOL {
            HWND sv = ::FindWindowExA(top, nullptr, "SHELLDLL_DefView", nullptr);
            if (sv) {
                HWND s = ::FindWindowExA(nullptr, top, "WorkerW", nullptr);
                if (s) { *reinterpret_cast<HWND*>(lp) = s; return FALSE; }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&result));
        if (result) return result;

        // Child-of-Progman pattern (Win11 layout). Pick the largest visible one.
        HWND child = ::FindWindowExA(progman, nullptr, "WorkerW", nullptr);
        HWND best = nullptr;
        int  bestArea = 0;
        while (child) {
            RECT r{};
            ::GetWindowRect(child, &r);
            int area = (r.right - r.left) * (r.bottom - r.top);
            if (::IsWindowVisible(child) && area > bestArea) {
                best = child;
                bestArea = area;
            }
            child = ::FindWindowExA(progman, child, "WorkerW", nullptr);
        }
        return best;
    }
};

} // namespace Platform
