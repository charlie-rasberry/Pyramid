#pragma once

// Single point where we pull in Windows headers, with the noise tamed.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Platform {

// Thin pass-throughs for the symbols we actually need elsewhere.
// (Keeping a layer here so the rest of the code doesn't need windows.h directly.)
using HwndType = HWND;

inline HWND FindWindowS(LPCSTR cls, LPCSTR name)              { return ::FindWindowA(cls, name); }
inline HWND FindWindowExS(HWND p, HWND ca, LPCSTR cls, LPCSTR n) { return ::FindWindowExA(p, ca, cls, n); }

} // namespace Platform
