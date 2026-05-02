// AsciiPyramid - DirectComposition build.
// The scene is picked at compile time in SceneSetup.h via ACTIVE_SCENE.

#include "../Platform/DcompRenderer.h"
#include "../Platform/DesktopWindow.h"
#include "../Platform/WorkerWManager.h"
#include "SceneSetup.h"

#include <chrono>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <combaseapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup") // for background usage.

//static constexpr int kBufferWidth  = 120;  // was 120
//static constexpr int kBufferHeight = 40;   // was 40

static volatile bool g_running = true;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT ||
        signal == CTRL_BREAK_EVENT) {
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

int main() {
    ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    HRESULT coHr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE) {
        std::printf("[diag] CoInitializeEx failed 0x%lX\n", (unsigned long)coHr);
        return 1;
    }

    HWND workerW = Platform::WorkerWManager::GetWorkerW();
    if (!workerW) {
        std::printf("Could not locate WorkerW.\n");
        return 1;
    }
    char cls[64] = {};
    ::GetClassNameA(workerW, cls, sizeof(cls));
    std::printf("[diag] WorkerW hwnd=%p class=%s\n", (void*)workerW, cls);

    Platform::DesktopWindow window;
    if (!window.Create() || !window.AttachToWorkerW(workerW)) {
        std::printf("Window create/attach failed.\n");
        return 1;
    }
    std::printf("[diag] hwnd=%p size=%dx%d\n",
                (void*)window.hwnd, window.screenW, window.screenH);

    Platform::DcompRenderer dcomp;
    if (!dcomp.Initialize(window.screenW, window.screenH) ||
        !dcomp.BindToWindow(window.hwnd)) {
        std::printf("DComp init/bind failed.\n");
        return 1;
    }
    int bufW = (int)(window.screenW / dcomp.cellW);
    int bufH = (int)(window.screenH / dcomp.cellH);
    Engine::AsciiBuffer buffer(bufW, bufH);
    App::ActiveScene scene;
    scene.Init(bufW, bufH);

    auto last = std::chrono::high_resolution_clock::now();
    uint64_t lastFrameHash = 0;
    int frames = 0;

    while (g_running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        if (!window.IsTargetValid()) {
            HWND fresh = Platform::WorkerWManager::GetWorkerW();
            if (fresh) {
                window.AttachToWorkerW(fresh);
                dcomp.BindToWindow(window.hwnd);
            }
        }

        scene.Update(dt);
        buffer.Clear();
        scene.Render(buffer);
        //dcomp.Render(buffer);

        uint64_t h = buffer.Hash();
        if (h != lastFrameHash) {
            dcomp.Render(buffer);
            lastFrameHash = h;
        } else {
            ::Sleep(8);
        }

        if (++frames % 120 == 0) {
            std::printf("[diag] %d frames\n", frames);
            std::fflush(stdout);
        }

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    if (HWND p = ::FindWindowA("Progman", nullptr)) {
        ::InvalidateRect(p, nullptr, TRUE);
        ::UpdateWindow(p);
    }
    ::CoUninitialize();
    return 0;
}
